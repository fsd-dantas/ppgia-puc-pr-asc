/*
 * jitter.c — Caracterização de Jitter: T1 periódica de alta prioridade
 *
 * Ambiente A (GPOS): Linux + pthreads com SCHED_FIFO e mlockall()
 *
 * Modelo de tarefas:
 *   T1  Alta     100 ms  Leve    (esta tarefa — mede jitter)
 *   T2  Média    200 ms  Média   (simulada por thread de carga moderada)
 *   T3  Baixa    500 ms  Pesada  (simulada por thread de carga intensa)
 *
 * Saída: ../../dados/linux/jitter.csv  (iteration, expected_ns, actual_ns, jitter_ns)
 *
 * Compilação:
 *   gcc -O2 -Wall -o jitter jitter.c -lm -lpthread
 *
 * Execução (requer CAP_SYS_NICE ou root para SCHED_FIFO):
 *   sudo ./jitter
 */

#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

/* ── Parâmetros ─────────────────────────────────────────────── */
#define ITERATIONS    1000
#define PERIOD_T1_NS  100000000LL   /* 100 ms em nanosegundos */
#define PERIOD_T2_NS  200000000LL   /* 200 ms */
#define PERIOD_T3_NS  500000000LL   /* 500 ms */
#define NSEC_PER_SEC  1000000000LL

#define OUTPUT_CSV    "../../dados/linux/jitter.csv"

/* ── Estado compartilhado ───────────────────────────────────── */
static volatile int running = 1;

typedef struct {
    int64_t expected_ns;
    int64_t actual_ns;
    int64_t jitter_ns;
} sample_t;

static sample_t samples[ITERATIONS];

/* ── Utilitários de tempo ───────────────────────────────────── */
static inline int64_t ts_to_ns(const struct timespec *ts)
{
    return (int64_t)ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

static inline void ns_to_ts(int64_t ns, struct timespec *ts)
{
    ts->tv_sec  = ns / NSEC_PER_SEC;
    ts->tv_nsec = ns % NSEC_PER_SEC;
}

/* ── Threads de carga ───────────────────────────────────────── */

/* T2: carga moderada — 200 ms de trabalho leve periódico */
static void *task_t2(void *arg)
{
    (void)arg;
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (running) {
        int64_t t = ts_to_ns(&next) + PERIOD_T2_NS;
        ns_to_ts(t, &next);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

        /* Carga média: laço com operações de ponto flutuante */
        volatile double acc = 0.0;
        for (int i = 0; i < 500000; i++)
            acc += (double)i * 0.000001;
        (void)acc;
    }
    return NULL;
}

/* T3: carga pesada — CPU burn contínuo */
static void *task_t3(void *arg)
{
    (void)arg;
    volatile double x = 1.0;
    while (running)
        x = x * 1.0000001 + 0.0000001;
    return NULL;
}

/* ── T1: tarefa de alta prioridade — mede jitter ────────────── */
static void *task_t1(void *arg)
{
    (void)arg;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    int64_t next_ns = ts_to_ns(&now) + PERIOD_T1_NS;

    for (int i = 0; i < ITERATIONS; i++) {
        struct timespec abs;
        ns_to_ts(next_ns, &abs);

        int ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &abs, NULL);
        if (ret && ret != EINTR) {
            fprintf(stderr, "clock_nanosleep erro: %s\n", strerror(ret));
            break;
        }

        struct timespec actual;
        clock_gettime(CLOCK_MONOTONIC, &actual);
        int64_t actual_ns = ts_to_ns(&actual);

        samples[i].expected_ns = next_ns;
        samples[i].actual_ns   = actual_ns;
        samples[i].jitter_ns   = actual_ns - next_ns;

        next_ns += PERIOD_T1_NS;
    }

    running = 0;
    return NULL;
}

/* ── Estatísticas ───────────────────────────────────────────── */
static void print_stats(void)
{
    int64_t sum = 0, max_j = INT64_MIN, min_j = INT64_MAX;

    for (int i = 0; i < ITERATIONS; i++) {
        int64_t j = samples[i].jitter_ns;
        sum += j;
        if (j > max_j) max_j = j;
        if (j < min_j) min_j = j;
    }

    double mean = (double)sum / ITERATIONS;
    double var  = 0.0;
    for (int i = 0; i < ITERATIONS; i++) {
        double d = (double)samples[i].jitter_ns - mean;
        var += d * d;
    }
    double stddev = sqrt(var / ITERATIONS);

    /* Percentil 99 — ordena cópia */
    int64_t sorted[ITERATIONS];
    for (int i = 0; i < ITERATIONS; i++)
        sorted[i] = samples[i].jitter_ns;
    /* insertion sort — simples, N=1000 */
    for (int i = 1; i < ITERATIONS; i++) {
        int64_t key = sorted[i];
        int j = i - 1;
        while (j >= 0 && sorted[j] > key) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }
    int64_t p99  = sorted[(int)(0.99  * ITERATIONS)];
    int64_t p999 = sorted[(int)(0.999 * ITERATIONS)];

    printf("\n=== Estatísticas de Jitter (T1, %d iterações) ===\n", ITERATIONS);
    printf("  Média     : %8.1f ns  (%.3f µs)\n", mean,   mean   / 1000.0);
    printf("  StdDev    : %8.1f ns  (%.3f µs)\n", stddev, stddev / 1000.0);
    printf("  Mínimo    : %8" PRId64 " ns  (%.3f µs)\n", min_j, min_j / 1000.0);
    printf("  Máximo    : %8" PRId64 " ns  (%.3f µs)\n", max_j, max_j / 1000.0);
    printf("  P99       : %8" PRId64 " ns  (%.3f µs)\n", p99,   p99   / 1000.0);
    printf("  P99.9     : %8" PRId64 " ns  (%.3f µs)\n", p999,  p999  / 1000.0);
}

/* ── Main ───────────────────────────────────────────────────── */
int main(void)
{
    /* Bloqueia paginação para evitar falhas de página durante medição */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
        fprintf(stderr, "aviso: mlockall falhou (%s) — prosseguindo sem lock\n",
                strerror(errno));

    pthread_t t1, t2, t3;
    pthread_attr_t attr_rt, attr_normal;
    struct sched_param sp;

    /* Atributos para T1 — SCHED_FIFO, prioridade máxima */
    pthread_attr_init(&attr_rt);
    pthread_attr_setschedpolicy(&attr_rt, SCHED_FIFO);
    sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr_rt, &sp);
    pthread_attr_setinheritsched(&attr_rt, PTHREAD_EXPLICIT_SCHED);

    /* Atributos para T2/T3 — SCHED_OTHER (CFS padrão) */
    pthread_attr_init(&attr_normal);
    pthread_attr_setschedpolicy(&attr_normal, SCHED_OTHER);
    sp.sched_priority = 0;
    pthread_attr_setschedparam(&attr_normal, &sp);

    printf("Iniciando coleta: %d iterações, período T1 = %lld ms\n",
           ITERATIONS, (long long)(PERIOD_T1_NS / 1000000));
    printf("Carga de fundo: T2 (moderada) + T3 (pesada)\n\n");

    pthread_create(&t2, &attr_normal, task_t2, NULL);
    pthread_create(&t3, &attr_normal, task_t3, NULL);
    pthread_create(&t1, &attr_rt,     task_t1, NULL);

    pthread_attr_destroy(&attr_rt);
    pthread_attr_destroy(&attr_normal);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    /* Escreve CSV */
    FILE *f = fopen(OUTPUT_CSV, "w");
    if (!f) {
        perror("fopen " OUTPUT_CSV);
        return EXIT_FAILURE;
    }
    fprintf(f, "iteration,expected_ns,actual_ns,jitter_ns\n");
    for (int i = 0; i < ITERATIONS; i++) {
        fprintf(f, "%d,%" PRId64 ",%" PRId64 ",%" PRId64 "\n",
                i,
                samples[i].expected_ns,
                samples[i].actual_ns,
                samples[i].jitter_ns);
    }
    fclose(f);
    printf("CSV salvo em: %s\n", OUTPUT_CSV);

    print_stats();

    return EXIT_SUCCESS;
}
