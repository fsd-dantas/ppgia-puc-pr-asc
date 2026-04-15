# Análise Avançada de RTOS: Determinismo e Inversão de Prioridade

**Disciplina:** Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR  
**Tema:** Caracterização Estatística de Determinismo e Análise de Inversão de Prioridade: GPOS vs. RTOS

## Objetivo

1. Caracterizar estatisticamente o **jitter** e a latência de ativação sob carga.
2. Investigar o fenômeno da **Inversão de Prioridade** e a eficácia de PIP/PCP.
3. Validar a **Teoria de Escalonamento de Liu & Layland (Rate Monotonic)** em ambientes reais e simulados.

## Ambientes

| Ambiente | Plataforma |
|---|---|
| **A — GPOS** | Linux Ubuntu 24.04 + pthreads (`SCHED_FIFO`, `mlockall()`) |
| **B — RTOS** | Zephyr emulado via QEMU (`qemu_x86`) |

## Modelo de Tarefas

| Tarefa | Prioridade | Período (Pi) | Carga (Ci) | Recurso |
|---|---|---|---|---|
| T1 | Alta | 100 ms | Leve | Acessa S1 (mutex) |
| T2 | Média | 200 ms | Média | Independente |
| T3 | Baixa | 500 ms | Pesada | Acessa S1 (mutex) |

**Limite RMS:** `U = Σ(Ci/Pi) ≤ n(2^(1/n) − 1)` → para n=3: U ≤ 0,7798

## Estrutura

```
rtos-determinismo/
├── README.md              → este arquivo
├── relatorio.md           → relatório técnico (entregável — short paper)
├── dados/
│   ├── linux/             → CSVs de latência coletados no GPOS
│   └── rtos/              → CSVs de latência coletados no RTOS/QEMU
└── src/
    ├── linux/             → código C com pthreads (Ambiente A)
    │   ├── jitter.c       → coleta de latência T1 (≥1000 iterações)
    │   └── priority_inversion.c  → cenário T3→S1→T2 preempta→T1 bloqueada
    └── rtos/              → código Zephyr (Ambiente B)
        ├── CMakeLists.txt
        ├── prj.conf
        └── src/
            ├── main.c     → setup das tasks e mutexes
            ├── jitter.c   → coleta de latência T1
            └── priority_inversion.c  → demonstração com Priority Inheritance
```

## Requisitos de Análise

### 4.1 Jitter
- Coletar tempo de início de T1 por ≥ 1.000 iterações em ambos os ambientes
- Entregável: histograma de latência Linux vs. RTOS sob estresse de CPU
- Discutir: desvio padrão e cauda longa (latências extremas)

### 4.2 Inversão de Prioridade
- Forçar: T3 segura S1 → T2 preempta T3 → T1 bloqueada
- Entregável: demonstrar inversão ilimitada no Linux; resolução via Priority Inheritance no Zephyr

### 4.3 Validação RMS
- Calcular limite teórico de escalonabilidade para o conjunto de tarefas
- Se ultrapassado: identificar qual tarefa perde deadline primeiro e como cada escalonador reage

## Execução

Ver [relatorio.md](relatorio.md) para metodologia completa, comandos de build e resultados.
