/*
 * main.c — Análise de RTOS: Jitter e Inversão de Prioridade
 *
 * Ambiente B (RTOS): Zephyr emulado via QEMU (qemu_x86)
 *
 * Este arquivo implementa dois experimentos em sequência:
 *
 *   1. Jitter (seção JITTER)
 *      T1 acorda periodicamente a cada 100 ms usando sleep absoluto.
 *      T2 e T3 executam carga de fundo.
 *      1.000 amostras de (esperado, real, jitter) são impressas como CSV.
 *
 *   2. Inversão de Prioridade (seção INVERSION)
 *      Rodada A — sem herança: usa k_sem como mutex binário (sem PIP).
 *      Rodada B — com herança: usa k_mutex (PIP habilitado por padrão no Zephyr).
 *      Cada rodada registra eventos com timestamp e imprime CSV.
 *
 * Saída: todo printk() vai para UART0 do QEMU (stdout do processo qemu).
 *
 * Build:
 *   west build -p auto -b qemu_x86 .
 *
 * Execução (capturando CSV):
 *   west build -t run 2>/dev/null | tee ../../../../dados/rtos/raw_output.txt
 *
 * Extrair CSV do jitter:
 *   grep "^JITTER," dados/rtos/raw_output.txt > dados/rtos/jitter.csv
 *
 * Extrair CSV da inversão:
 *   grep "^INV," dados/rtos/raw_output.txt > dados/rtos/priority_inversion.csv
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* ════════════════════════════════════════════════════════════════
 * Configuração global
 * ════════════════════════════════════════════════════════════════ */

/* Período de T1 em ms e ticks */
#define PERIOD_MS        100
#define PERIOD_TICKS     k_ms_to_ticks_ceil32(PERIOD_MS)

/* Número de amostras de jitter */
#define JITTER_ITERS     1000

/* Prioridades Zephyr: menor número = maior prioridade (preemptiva) */
#define PRIO_T1   2    /* alta  */
#define PRIO_T2   6    /* média */
#define PRIO_T3   10   /* baixa */

/* Cargas de CPU (em ticks) */
#define T3_HOLD_TICKS    k_ms_to_ticks_ceil32(80)   /* 80 ms segurando lock  */
#define T2_WORK_TICKS    k_ms_to_ticks_ceil32(60)   /* 60 ms trabalho de T2  */

#define STACK_SIZE  1024

/* ── Utilitário: tempo atual em µs ─────────────────────────── */
static inline int64_t now_us(void)
{
    return k_ticks_to_us_floor64(k_uptime_ticks());
}

/* ── Utilitário: CPU burn por n ticks de wall clock ─────────── */
static void cpu_burn_ticks(k_ticks_t duration)
{
    k_ticks_t end = k_uptime_ticks() + duration;
    while (k_uptime_ticks() < end) {
        /* spin — não cede a CPU */
    }
}

/* ════════════════════════════════════════════════════════════════
 * SEÇÃO 1 — JITTER
 * ════════════════════════════════════════════════════════════════ */

/* Amostras de jitter (em µs) */
static int64_t jitter_expected_us[JITTER_ITERS];
static int64_t jitter_actual_us[JITTER_ITERS];

/* Flag de conclusão do experimento */
static volatile int jitter_done = 0;

/* Stacks das threads de jitter */
K_THREAD_STACK_DEFINE(jitter_t1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(jitter_t2_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(jitter_t3_stack, STACK_SIZE);

static struct k_thread jitter_t1_data;
static struct k_thread jitter_t2_data;
static struct k_thread jitter_t3_data;

/* T2 — carga moderada: loop de trabalho periódico */
static void jitter_t2_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while (!jitter_done) {
        cpu_burn_ticks(k_ms_to_ticks_ceil32(5));  /* 5 ms de trabalho */
        k_sleep(K_MSEC(15));                       /* 15 ms dormindo  */
    }
}

/* T3 — carga pesada: CPU burn contínuo */
static void jitter_t3_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    while (!jitter_done) {
        cpu_burn_ticks(k_ms_to_ticks_ceil32(20));
        k_yield();  /* evita starvation total — cede brevemente */
    }
}

/* T1 — alta prioridade: mede jitter de ativação */
static void jitter_t1_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_ticks_t next_tick = k_uptime_ticks() + PERIOD_TICKS;

    for (int i = 0; i < JITTER_ITERS; i++) {
        int64_t expected_us = k_ticks_to_us_floor64(next_tick);

        /* Sleep absoluto: similar a clock_nanosleep(TIMER_ABSTIME) */
        k_sleep(K_TIMEOUT_ABS_TICKS(next_tick));

        int64_t actual_us = now_us();

        jitter_expected_us[i] = expected_us;
        jitter_actual_us[i]   = actual_us;

        next_tick += PERIOD_TICKS;
    }

    jitter_done = 1;
}

static void experiment_jitter(void)
{
    printk("\n# === EXPERIMENTO 1: JITTER ===\n");
    printk("# Periodo T1: %d ms | Iteracoes: %d\n", PERIOD_MS, JITTER_ITERS);
    printk("# Carga de fundo: T2 (moderada) + T3 (pesada)\n");
    printk("# Coluna: JITTER,iteration,expected_us,actual_us,jitter_us\n\n");

    jitter_done = 0;

    k_tid_t tid1 = k_thread_create(&jitter_t1_data, jitter_t1_stack,
                                    K_THREAD_STACK_SIZEOF(jitter_t1_stack),
                                    jitter_t1_fn, NULL, NULL, NULL,
                                    PRIO_T1, 0, K_NO_WAIT);

    k_tid_t tid2 = k_thread_create(&jitter_t2_data, jitter_t2_stack,
                                    K_THREAD_STACK_SIZEOF(jitter_t2_stack),
                                    jitter_t2_fn, NULL, NULL, NULL,
                                    PRIO_T2, 0, K_NO_WAIT);

    k_tid_t tid3 = k_thread_create(&jitter_t3_data, jitter_t3_stack,
                                    K_THREAD_STACK_SIZEOF(jitter_t3_stack),
                                    jitter_t3_fn, NULL, NULL, NULL,
                                    PRIO_T3, 0, K_NO_WAIT);

    k_thread_name_set(tid1, "T1-jitter");
    k_thread_name_set(tid2, "T2-load");
    k_thread_name_set(tid3, "T3-load");

    k_thread_join(&jitter_t1_data, K_FOREVER);

    /* Encerra T2 e T3 */
    k_thread_abort(tid2);
    k_thread_abort(tid3);

    /* Imprime CSV */
    printk("JITTER,iteration,expected_us,actual_us,jitter_us\n");
    for (int i = 0; i < JITTER_ITERS; i++) {
        printk("JITTER,%d,%lld,%lld,%lld\n",
               i,
               (long long)jitter_expected_us[i],
               (long long)jitter_actual_us[i],
               (long long)(jitter_actual_us[i] - jitter_expected_us[i]));
    }

    printk("\n# Jitter CSV concluido.\n\n");
}

/* ════════════════════════════════════════════════════════════════
 * SEÇÃO 2 — INVERSÃO DE PRIORIDADE
 * ════════════════════════════════════════════════════════════════ */

/*
 * No Zephyr, k_mutex SEMPRE usa priority inheritance (comportamento padrão
 * e não configurável por-mutex). Para demonstrar inversão SEM herança,
 * usamos k_sem como mutex binário (k_sem NÃO tem PIP).
 *
 *   Rodada A (sem PIP): k_sem(1,1) como lock exclusivo
 *   Rodada B (com PIP): k_mutex com herança automática
 */

/* Semáforo e mutex para S1 */
static K_SEM_DEFINE(s1_sem, 1, 1);
static K_MUTEX_DEFINE(s1_mutex);

/* Variante do lock em uso na rodada corrente */
typedef enum { LOCK_SEM = 0, LOCK_MUTEX = 1 } lock_type_t;

static lock_type_t current_lock;

static inline void lock_s1(void)
{
    if (current_lock == LOCK_SEM)
        k_sem_take(&s1_sem, K_FOREVER);
    else
        k_mutex_lock(&s1_mutex, K_FOREVER);
}

static inline void unlock_s1(void)
{
    if (current_lock == LOCK_SEM)
        k_sem_give(&s1_sem);
    else
        k_mutex_unlock(&s1_mutex);
}

/* Stacks das threads de inversão */
K_THREAD_STACK_DEFINE(inv_t1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(inv_t2_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(inv_t3_stack, STACK_SIZE);

static struct k_thread inv_t1_data;
static struct k_thread inv_t2_data;
static struct k_thread inv_t3_data;

/* Sincronização de início: T3 adquire S1, depois T1/T2 iniciam */
static K_SEM_DEFINE(inv_s1_acquired, 0, 1);  /* T3 posta quando tem S1 */
static volatile int inv_done = 0;

/* T3 — baixa prioridade: adquire S1, faz CPU burn longo */
static void inv_t3_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    int64_t t = now_us();
    printk("INV,%lld,T3,adquirindo S1\n", (long long)t);

    lock_s1();

    t = now_us();
    printk("INV,%lld,T3,adquiriu S1 - iniciando trabalho pesado\n", (long long)t);

    /* Sinaliza que S1 foi adquirido — T1 e T2 podem começar */
    k_sem_give(&inv_s1_acquired);

    /* Trabalho pesado enquanto segura S1 */
    cpu_burn_ticks(T3_HOLD_TICKS);

    t = now_us();
    printk("INV,%lld,T3,trabalho concluido - liberando S1\n", (long long)t);

    unlock_s1();

    t = now_us();
    printk("INV,%lld,T3,fim\n", (long long)t);

    inv_done = 1;
}

/* T1 — alta prioridade: espera S1 ser adquirido por T3, depois tenta */
static void inv_t1_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    /* Aguarda T3 ter S1 para garantir a sequência correta */
    k_sem_take(&inv_s1_acquired, K_FOREVER);
    /* Recoloca para T2 também consumir */
    k_sem_give(&inv_s1_acquired);

    int64_t t_try = now_us();
    printk("INV,%lld,T1,tentando adquirir S1\n", (long long)t_try);

    lock_s1();

    int64_t t_got = now_us();
    printk("INV,%lld,T1,adquiriu S1 - bloqueada por %lld us\n",
           (long long)t_got, (long long)(t_got - t_try));

    unlock_s1();

    printk("INV,%lld,T1,fim\n", (long long)now_us());
}

/* T2 — média prioridade: trabalho independente */
static void inv_t2_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_sem_take(&inv_s1_acquired, K_FOREVER);
    k_sem_give(&inv_s1_acquired);

    printk("INV,%lld,T2,iniciou trabalho medio\n", (long long)now_us());
    cpu_burn_ticks(T2_WORK_TICKS);
    printk("INV,%lld,T2,fim\n", (long long)now_us());
}

static void run_inversion_scenario(lock_type_t lock_type)
{
    const char *label = (lock_type == LOCK_SEM)
        ? "no_pip (k_sem)"
        : "pip   (k_mutex)";

    printk("\n# --- Cenario: %s ---\n", label);
    printk("# INV format: INV,wall_us,task,event\n\n");

    current_lock = lock_type;
    inv_done     = 0;

    /* Reset semáforo de sincronização */
    k_sem_reset(&inv_s1_acquired);

    /* Reset do lock de S1 */
    if (lock_type == LOCK_SEM) {
        k_sem_give(&s1_sem);        /* garante disponível */
    }
    /* k_mutex não precisa reset — já está desbloqueado */

    /* T3 primeiro (menor prioridade) — adquire S1 antes de T1/T2 tentarem */
    k_tid_t tid3 = k_thread_create(&inv_t3_data, inv_t3_stack,
                                    K_THREAD_STACK_SIZEOF(inv_t3_stack),
                                    inv_t3_fn, NULL, NULL, NULL,
                                    PRIO_T3, 0, K_NO_WAIT);

    /* Aguarda T3 ter S1 antes de criar T1 e T2 */
    k_sem_take(&inv_s1_acquired, K_FOREVER);
    k_sem_give(&inv_s1_acquired);

    k_tid_t tid1 = k_thread_create(&inv_t1_data, inv_t1_stack,
                                    K_THREAD_STACK_SIZEOF(inv_t1_stack),
                                    inv_t1_fn, NULL, NULL, NULL,
                                    PRIO_T1, 0, K_NO_WAIT);

    k_tid_t tid2 = k_thread_create(&inv_t2_data, inv_t2_stack,
                                    K_THREAD_STACK_SIZEOF(inv_t2_stack),
                                    inv_t2_fn, NULL, NULL, NULL,
                                    PRIO_T2, 0, K_NO_WAIT);

    k_thread_name_set(tid1, "T1-inv");
    k_thread_name_set(tid2, "T2-inv");
    k_thread_name_set(tid3, "T3-inv");

    k_thread_join(&inv_t1_data, K_FOREVER);
    k_thread_join(&inv_t2_data, K_FOREVER);
    k_thread_join(&inv_t3_data, K_FOREVER);

    printk("\n# Cenario %s concluido.\n", label);
}

/* ════════════════════════════════════════════════════════════════
 * Entry point
 * ════════════════════════════════════════════════════════════════ */

int main(void)
{
    printk("\n");
    printk("##############################################\n");
    printk("# RTOS Determinismo — Zephyr / QEMU (qemu_x86)\n");
    printk("# Tick rate: %d Hz | Resolucao: %d us/tick\n",
           CONFIG_SYS_CLOCK_TICKS_PER_SEC,
           1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
    printk("##############################################\n\n");

    /* Experimento 1: Jitter */
    experiment_jitter();

    /* Experimento 2: Inversão de Prioridade */
    printk("\n# === EXPERIMENTO 2: INVERSAO DE PRIORIDADE ===\n");
    printk("# T1 prio=%d (alta) | T2 prio=%d (media) | T3 prio=%d (baixa)\n",
           PRIO_T1, PRIO_T2, PRIO_T3);
    printk("# T3 hold: %d ms (CPU burn) | T2 work: %d ms\n\n",
           80, 60);

    run_inversion_scenario(LOCK_SEM);    /* sem PIP */
    run_inversion_scenario(LOCK_MUTEX);  /* com PIP */

    printk("\n##############################################\n");
    printk("# Todos os experimentos concluidos.\n");
    printk("##############################################\n");

    return 0;
}
