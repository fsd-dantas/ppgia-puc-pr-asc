/*
 * priority_inversion.c — Demonstração de Inversão de Prioridade
 *
 * Cenário: T3 (baixa) adquire S1 e realiza trabalho pesado.
 *          T1 (alta) tenta S1 → bloqueia.
 *          T2 (média) preempta T3 (sem PIP), atrasando T1 indefinidamente.
 *
 * Rodadas executadas em sequência:
 *   1. SEM herança  (PTHREAD_PRIO_NONE)    → inversão ilimitada
 *   2. COM herança  (PTHREAD_PRIO_INHERIT) → T3 herda prioridade de T1
 *
 * Mecanismo do hold: T3 faz CPU burn medido por CLOCK_THREAD_CPUTIME_ID.
 *   Sem PIP: T2 preempta T3 → contador de CPU de T3 pausa → T3 demora mais.
 *   Com PIP: T3 herda prio de T1 → T2 não consegue preemptar → T3 termina rápido.
 *
 * Saída: ../../dados/linux/priority_inversion.csv
 *
 * Compilação:
 *   gcc -O2 -Wall -o priority_inversion priority_inversion.c -lpthread
 *
 * Execução (requer CAP_SYS_NICE ou root para SCHED_FIFO):
 *   sudo ./priority_inversion
 */

#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

/* ── Prioridades SCHED_FIFO ──────────────────────────────────── */
#define PRIO_T1  90   /* alta  */
#define PRIO_T2  50   /* média */
#define PRIO_T3  10   /* baixa */

/* ── Temporizações ────────────────────────────────────────────── */
#define T3_INITIAL_SLEEP_NS   10000000LL  /*  10 ms — garante que T3 pegue S1 antes de T1/T2 */
#define T1_T2_SLEEP_NS        20000000LL  /*  20 ms — T1 e T2 dormem antes de disputar */
#define T3_HOLD_CPU_NS        80000000LL  /*  80 ms de CPU de T3 segurando S1 */
#define T2_WORK_CPU_NS        60000000LL  /*  60 ms de CPU de T2 */

#define NSEC_PER_SEC  1000000000LL
#define OUTPUT_CSV    "../../dados/linux/priority_inversion.csv"
#define MAX_LOG       64

/* ── Log de eventos ─────────────────────────────────────────── */
typedef struct {
    int64_t     wall_us;
    const char *task;
    const char *event;
} log_entry_t;

static log_entry_t log_buf[MAX_LOG];
static int         log_count;
static int64_t     t_scenario_start;

static inline int64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
}

static void log_event(const char *task, const char *event)
{
    int64_t us = (now_ns() - t_scenario_start) / 1000;
    if (log_count < MAX_LOG) {
        log_buf[log_count].wall_us = us;
        log_buf[log_count].task    = task;
        log_buf[log_count].event   = event;
        log_count++;
    }
    printf("  t+%6" PRId64 " µs  [%-2s] %s\n", us, task, event);
    fflush(stdout);
}

/* ── CPU burn baseado em tempo de CPU da thread ────────────── */
static void cpu_burn_thread_ns(int64_t duration_ns)
{
    struct timespec start, now;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
    int64_t end = (int64_t)start.tv_sec * NSEC_PER_SEC + start.tv_nsec + duration_ns;
    do {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now);
    } while ((int64_t)now.tv_sec * NSEC_PER_SEC + now.tv_nsec < end);
}

/* ── Contexto compartilhado por cenário ─────────────────────── */
typedef struct {
    pthread_mutex_t s1;
} scenario_ctx_t;

/* ── T3 — baixa prioridade, segura S1 ──────────────────────── */
static void *task_t3(void *arg)
{
    scenario_ctx_t *ctx = arg;

    /* Deixa T1 e T2 serem criadas e chamarem nanosleep antes de agir */
    struct timespec sl = { .tv_sec = 0, .tv_nsec = T3_INITIAL_SLEEP_NS };
    nanosleep(&sl, NULL);

    log_event("T3", "tentando adquirir S1");
    pthread_mutex_lock(&ctx->s1);
    log_event("T3", "adquiriu S1 — trabalho pesado (CPU burn)");

    /* Segura S1 pelo tempo de CPU especificado.
     * Sem PIP: T2 preempta → este burn demora mais em wall clock.
     * Com PIP: T3 herda prio de T1 → T2 não preempta → burn contínuo. */
    cpu_burn_thread_ns(T3_HOLD_CPU_NS);

    log_event("T3", "trabalho concluído — liberando S1");
    pthread_mutex_unlock(&ctx->s1);
    log_event("T3", "fim");
    return NULL;
}

/* ── T1 — alta prioridade, precisa de S1 ───────────────────── */
static void *task_t1(void *arg)
{
    scenario_ctx_t *ctx = arg;

    struct timespec sl = { .tv_sec = 0, .tv_nsec = T1_T2_SLEEP_NS };
    nanosleep(&sl, NULL);

    log_event("T1", "tentando adquirir S1");
    int64_t t_blocked = now_ns();

    pthread_mutex_lock(&ctx->s1);
    int64_t blocked_us = (now_ns() - t_blocked) / 1000;

    log_event("T1", "adquiriu S1");
    printf("  >>>  T1 ficou bloqueada por %" PRId64 " µs (%.1f ms)\n\n",
           blocked_us, blocked_us / 1000.0);

    pthread_mutex_unlock(&ctx->s1);
    log_event("T1", "fim");
    return NULL;
}

/* ── T2 — média prioridade, trabalho independente ──────────── */
static void *task_t2(void *arg)
{
    (void)arg;

    struct timespec sl = { .tv_sec = 0, .tv_nsec = T1_T2_SLEEP_NS };
    nanosleep(&sl, NULL);

    log_event("T2", "iniciou trabalho médio (CPU burn)");
    cpu_burn_thread_ns(T2_WORK_CPU_NS);
    log_event("T2", "fim");
    return NULL;
}

/* ── Criação de thread SCHED_FIFO ───────────────────────────── */
static void create_rt_thread(pthread_t *tid, int prio,
                              void *(*fn)(void *), void *arg)
{
    pthread_attr_t     attr;
    struct sched_param sp = { .sched_priority = prio };

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &sp);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    int ret = pthread_create(tid, &attr, fn, arg);
    pthread_attr_destroy(&attr);

    if (ret) {
        fprintf(stderr, "pthread_create (prio=%d): %s\n", prio, strerror(ret));
        fprintf(stderr, "Dica: execute com 'sudo' para permissão SCHED_FIFO.\n");
        exit(EXIT_FAILURE);
    }
}

/* ── Execução de um cenário completo ────────────────────────── */
typedef struct {
    log_entry_t entries[MAX_LOG];
    int         count;
    const char *label;
} scenario_result_t;

static void run_scenario(int use_pip, scenario_result_t *result)
{
    result->label = use_pip
        ? "pip"
        : "no_pip";

    const char *label_print = use_pip
        ? "COM Priority Inheritance (PTHREAD_PRIO_INHERIT)"
        : "SEM herança de prioridade (PTHREAD_PRIO_NONE)";

    printf("\n══════════════════════════════════════════════════════\n");
    printf("  Cenário: %s\n", label_print);
    printf("══════════════════════════════════════════════════════\n\n");

    scenario_ctx_t ctx;

    /* Inicializa mutex com ou sem PIP */
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setprotocol(&mattr,
        use_pip ? PTHREAD_PRIO_INHERIT : PTHREAD_PRIO_NONE);
    pthread_mutex_init(&ctx.s1, &mattr);
    pthread_mutexattr_destroy(&mattr);

    log_count        = 0;
    t_scenario_start = now_ns();

    /* Criar T1 primeiro (maior prio) — chama nanosleep logo no início,
     * liberando a CPU para main criar T2 e T3 na sequência. */
    pthread_t t1, t2, t3;
    create_rt_thread(&t1, PRIO_T1, task_t1, &ctx);
    create_rt_thread(&t2, PRIO_T2, task_t2, &ctx);
    create_rt_thread(&t3, PRIO_T3, task_t3, &ctx);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    pthread_mutex_destroy(&ctx.s1);

    /* Salva resultado */
    memcpy(result->entries, log_buf, log_count * sizeof(log_entry_t));
    result->count = log_count;
}

/* ── Main ───────────────────────────────────────────────────── */
int main(void)
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
        fprintf(stderr, "aviso: mlockall: %s\n", strerror(errno));

    scenario_result_t r_no_pip, r_pip;

    run_scenario(0, &r_no_pip);
    run_scenario(1, &r_pip);

    /* Escreve CSV */
    FILE *f = fopen(OUTPUT_CSV, "w");
    if (!f) {
        perror("fopen " OUTPUT_CSV);
        return EXIT_FAILURE;
    }

    fprintf(f, "scenario,task,event,wall_us\n");

    for (int i = 0; i < r_no_pip.count; i++)
        fprintf(f, "%s,%s,\"%s\",%" PRId64 "\n",
                r_no_pip.label,
                r_no_pip.entries[i].task,
                r_no_pip.entries[i].event,
                r_no_pip.entries[i].wall_us);

    for (int i = 0; i < r_pip.count; i++)
        fprintf(f, "%s,%s,\"%s\",%" PRId64 "\n",
                r_pip.label,
                r_pip.entries[i].task,
                r_pip.entries[i].event,
                r_pip.entries[i].wall_us);

    fclose(f);
    printf("\nCSV salvo em: %s\n", OUTPUT_CSV);

    return EXIT_SUCCESS;
}
