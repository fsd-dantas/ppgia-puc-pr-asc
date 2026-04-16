# Análise do Hypervisor e do Impacto da Máquina Virtual sobre o Host

**Disciplina:** Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR  
**Repositório:** [github.com/fsd-dantas/ppgia-puc-pr-asc](https://github.com/fsd-dantas/ppgia-puc-pr-asc)

> Analisar o papel do hypervisor na mediação de recursos entre máquina virtual e sistema hospedeiro.

---

## Contextualização

Em ambientes virtualizados, o sistema operacional convidado executa sobre uma camada intermediária de virtualização, denominada hypervisor, responsável por mediar o acesso a CPU, memória, disco, rede e demais recursos físicos do sistema hospedeiro.

Diferentemente da execução nativa, em que a aplicação acessa diretamente os serviços do sistema operacional do host, a execução em máquina virtual envolve uma cadeia adicional de abstração. Assim, operações realizadas dentro da VM — como processamento, leitura e escrita em disco — podem produzir efeitos observáveis também no sistema hospedeiro.

Esta atividade tem como objetivo investigar, na prática, como o hypervisor intermedeia o uso de recursos e como cargas executadas no sistema convidado impactam o desempenho e o comportamento do host.

---

## Objetivo

Analisar o funcionamento do hypervisor por meio da execução de cargas de trabalho em uma máquina virtual, observando:

- o comportamento da VM durante a execução;
- o impacto da VM sobre o sistema hospedeiro;
- a relação entre as operações realizadas no guest e o consumo de recursos no host;
- o papel do hypervisor na abstração e mediação de recursos computacionais.

---

## Descrição Geral

Cada grupo deverá executar experimentos em um ambiente composto por um sistema hospedeiro (host) e uma máquina virtual (guest) em execução sobre um hypervisor.

A proposta não consiste apenas em comparar tempos de execução entre host e VM, mas em **entender o caminho percorrido pelas operações da VM** e identificar seus reflexos no host. Os grupos deverão aplicar cargas de trabalho dentro da VM e monitorar, simultaneamente, os efeitos observados tanto no guest quanto no host.

---

## Parte 1 — Caracterização do Ambiente

### Sistema hospedeiro

Informar: sistema operacional, processador, quantidade de memória RAM, tipo de armazenamento, número de núcleos disponíveis e software de virtualização utilizado.

### Máquina virtual

Informar: sistema operacional convidado, quantidade de vCPUs, memória RAM atribuída, tamanho do disco virtual, tipo de disco virtual (se conhecido) e outras configurações relevantes.

---

## Parte 2 — Experimentos

Realizar ao menos três experimentos. A **presença do experimento de escrita em disco é obrigatória**, pois permite observar com maior clareza o efeito da VM sobre o host.

### Experimento 1 — Carga de CPU dentro da VM

Executar uma tarefa computacional intensiva e observar: tempo de execução no guest, uso de CPU dentro da VM, consumo de CPU do processo da VM no host.

```bash
time python3 -c "sum(i*i for i in range(10**7))"
```

### Experimento 2 — Escrita em disco dentro da VM *(obrigatório)*

Executar uma operação de escrita e observar: tempo de escrita no guest, impacto no uso de disco do host, comportamento do processo da VM no host, efeito em cache e espera por I/O.

```bash
time dd if=/dev/zero of=teste.bin bs=1M count=512 conv=fdatasync
```

### Experimento 3 — Leitura em disco dentro da VM

Executar uma operação de leitura e observar: tempo de leitura no guest, atividade de leitura no host, diferença entre comportamento de leitura e de escrita.

```bash
time dd if=teste.bin of=/dev/null bs=1M
```

### Experimento 4 — Observação de chamadas de sistema

Executar um comando simples e observar as syscalls realizadas pelo guest, discutindo como essas operações dependem da mediação do hypervisor.

```bash
strace -c ls > /dev/null
strace -c cat teste.bin > /dev/null
```

---

## Parte 3 — Monitoramento Simultâneo

### No guest

- uso de CPU e memória
- tempo de execução
- atividade de disco
- chamadas de sistema (quando aplicável)

### No host

- uso de CPU do processo da máquina virtual
- uso de memória associado à VM
- atividade de leitura e escrita em disco
- espera por I/O
- carga geral do sistema durante a execução da VM

---

## Parte 4 — Ferramentas Sugeridas

### Informações gerais do sistema

```bash
uname -a
lscpu
free -h
cat /proc/meminfo
cat /proc/cpuinfo
```

### Monitoramento de processos e CPU

```bash
top
ps -eo pid,ppid,comm,%cpu,%mem --sort=-%cpu | head
pidstat 1
```

### Monitoramento de disco e I/O

```bash
iostat -xz 1
iotop
vmstat 1
dstat
```

### Análise de chamadas de sistema

```bash
strace -c ls > /dev/null
```

### Informações de processos via `/proc`

```bash
cat /proc/self/status
cat /proc/<pid>/status
cat /proc/diskstats
```

### Medição de tempo

```bash
time dd if=/dev/zero of=teste.bin bs=1M count=512 conv=fdatasync
```

### Processo da VM no host

Identificar o processo correspondente à VM e acompanhá-lo com `ps`, `top`, `htop` ou `pidstat`.

---

## Parte 5 — Organização dos Resultados

Registrar os resultados em tabelas com as colunas: **experimento**, **métrica observada**, **valor no guest**, **valor no host**, **observações**.

Repetir cada experimento ao menos **3 vezes** e reportar a mediana.

---

## Parte 6 — Análise e Discussão

A análise técnica deve responder, no mínimo:

1. Como o host foi impactado por cargas executadas dentro da VM?
2. O processo da VM apresentou aumento de uso de CPU e/ou memória durante os testes?
3. A escrita em disco no guest produziu atividade perceptível no host?
4. Houve diferença entre o comportamento de leitura e escrita?
5. O desempenho observado depende apenas do sistema convidado ou também da camada de virtualização?
6. Que evidências obtidas indicam a atuação do hypervisor como intermediador de recursos?
7. Quais limitações experimentais podem ter influenciado os resultados?

---

## Entregável

Relatório técnico contendo: identificação dos integrantes, descrição do ambiente, descrição dos experimentos, ferramentas utilizadas, resultados obtidos, análise comparativa guest vs. host e conclusão.

Ver [relatorio.md](relatorio.md) para o template completo.

### Critérios de avaliação

| Critério | Peso |
|---|---|
| Metodologia | 15% |
| Uso das ferramentas | 15% |
| Apresentação dos resultados | 15% |
| Relação dos resultados com o papel do hypervisor | 25% |
| Análise técnica | 20% |
| Conclusões | 10% |

---

## Estrutura

```
hypervisor-vm-host/
├── README.md              → este arquivo (enunciado da atividade)
├── relatorio.md           → relatório técnico (entregável)
├── setup.md               → registro dos procedimentos de ambiente
├── dados/                 → saídas brutas dos experimentos (não versionado)
│   ├── exp1-cpu/
│   ├── exp2-disco-escrita/
│   ├── exp3-disco-leitura/
│   └── exp4-syscalls/
└── scripts/
    └── coleta.sh          → script auxiliar de coleta de baseline
```
