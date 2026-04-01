# Relatório Técnico — Análise do Hipervisor e do Impacto da Máquina Virtual sobre o Host

**Disciplina:** Arquitetura de Sistemas Computacionais  
**Programa:** PPGIA — Mestrado em Informática — PUC-PR  
**Integrantes:** <!-- TODO: nomes -->  
**Data:** <!-- TODO: data de entrega -->

---

## 1. Caracterização do Ambiente

### 1.1 Sistema Hospedeiro (Host)

| Atributo | Valor |
|---|---|
| Sistema Operacional | <!-- ex.: Windows 11 Home 23H2 --> |
| Processador | <!-- ex.: Intel Core i7-12700H --> |
| Memória RAM | <!-- ex.: 32 GB DDR5 --> |
| Armazenamento | <!-- ex.: SSD NVMe 1 TB --> |
| Núcleos disponíveis | <!-- ex.: 14 (6P + 8E) --> |
| Software de virtualização | <!-- ex.: VirtualBox 7.0 / VMware Workstation 17 / KVM --> |

### 1.2 Máquina Virtual (Guest)

| Atributo | Valor |
|---|---|
| Sistema Operacional convidado | <!-- ex.: Ubuntu 24.04 LTS --> |
| vCPUs atribuídas | <!-- ex.: 4 --> |
| Memória RAM atribuída | <!-- ex.: 8 GB --> |
| Tamanho do disco virtual | <!-- ex.: 50 GB --> |
| Tipo de disco virtual | <!-- ex.: VDI / VMDK / qcow2; alocação dinâmica --> |
| Outras configurações | <!-- ex.: VT-x habilitado, PAE/NX ativo --> |

---

## 2. Experimentos

> Cada experimento foi repetido **3 vezes**; os valores apresentados são a mediana das execuções.

### Experimento 1 — Carga de CPU dentro da VM

**Objetivo:** Executar uma tarefa computacional intensiva e comparar o consumo de CPU no guest e no processo da VM no host.

**Comando executado no guest:**
```bash
time python3 -c "sum(i*i for i in range(10**7))"
```

**Monitoramento no guest:**
```bash
# Em terminal separado, durante a execução:
top -b -n 5 -d 1 | grep python3
```

**Monitoramento no host:**
```bash
# Identificar PID do processo da VM:
ps -eo pid,ppid,comm,%cpu,%mem --sort=-%cpu | head
# Acompanhar:
pidstat -p <PID_VM> 1
```

**Resultados:**

| Métrica | Guest | Host | Observações |
|---|---|---|---|
| Tempo de execução (real) | | | |
| Uso de CPU (pico) | | | |
| Uso de memória | | | |

**Saída bruta:** [`dados/exp1-cpu/`](dados/exp1-cpu/)

---

### Experimento 2 — Escrita em disco dentro da VM (obrigatório)

**Objetivo:** Observar como a escrita no disco virtual se manifesta como atividade de I/O no host.

**Comando executado no guest:**
```bash
time dd if=/dev/zero of=teste.bin bs=1M count=512 conv=fdatasync
```

**Monitoramento no guest:**
```bash
iostat -xz 1 &
vmstat 1 &
```

**Monitoramento no host:**
```bash
iostat -xz 1   # atividade geral de disco
iotop          # processos com maior I/O
pidstat -d -p <PID_VM> 1
```

**Resultados:**

| Métrica | Guest | Host | Observações |
|---|---|---|---|
| Tempo de escrita (real) | | | |
| Throughput de escrita | | | |
| Espera por I/O (%iowait) | | | |
| Atividade de escrita no host (kB/s) | | | |

**Saída bruta:** [`dados/exp2-disco-escrita/`](dados/exp2-disco-escrita/)

---

### Experimento 3 — Leitura em disco dentro da VM

**Objetivo:** Comparar o comportamento de leitura em relação à escrita; avaliar efeito de cache.

**Pré-requisito:** arquivo `teste.bin` criado no Experimento 2.

**Comando executado no guest:**
```bash
# Limpar cache (se permissão root):
echo 3 | sudo tee /proc/sys/vm/drop_caches

time dd if=teste.bin of=/dev/null bs=1M
```

**Monitoramento:** idem Experimento 2.

**Resultados:**

| Métrica | Guest | Host | Observações |
|---|---|---|---|
| Tempo de leitura (real) | | | |
| Throughput de leitura | | | |
| Espera por I/O (%iowait) | | | |
| Atividade de leitura no host (kB/s) | | | |
| Diferença em relação à escrita | | | |

**Saída bruta:** [`dados/exp3-disco-leitura/`](dados/exp3-disco-leitura/)

---

### Experimento 4 — Observação de chamadas de sistema

**Objetivo:** Identificar as syscalls geradas por operações simples no guest e discutir a mediação pelo hipervisor.

**Comando executado no guest:**
```bash
strace -c ls > /dev/null
```

```bash
strace -c cat teste.bin > /dev/null
```

**Resultados — `strace -c ls`:**

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
<!-- TODO: colar saída aqui -->
```

**Resultados — `strace -c cat teste.bin`:**

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
<!-- TODO: colar saída aqui -->
```

**Saída bruta:** [`dados/exp4-syscalls/`](dados/exp4-syscalls/)

---

## 3. Monitoramento Simultâneo

### 3.1 Ferramentas utilizadas

| Ferramenta | Contexto | Finalidade |
|---|---|---|
| `top` / `htop` | Guest e Host | Uso de CPU e memória em tempo real |
| `iostat -xz 1` | Guest e Host | Atividade de disco, %iowait, throughput |
| `pidstat` | Host | CPU/I/O do processo específico da VM |
| `vmstat 1` | Guest | Memória, swap, I/O por bloco, CPU |
| `iotop` | Host | Processos com maior atividade de I/O |
| `strace -c` | Guest | Contagem de chamadas de sistema |
| `time` | Guest | Medição de tempo de execução |
| `/proc/<pid>/status` | Guest e Host | Estado e consumo de memória do processo |

### 3.2 Processo da VM no Host

```bash
# Identificação do processo da VM
ps -eo pid,ppid,comm,%cpu,%mem --sort=-%cpu | grep -E "VBoxHeadless|qemu|vmware"
```

PID identificado: `<!-- TODO -->`  
Processo: `<!-- ex.: VBoxHeadless / qemu-system-x86_64 -->`

---

## 4. Resultados Consolidados

| Experimento | Métrica observada | Valor no Guest | Valor no Host | Observações |
|---|---|---|---|---|
| 1 — CPU | Tempo real (s) | | | |
| 1 — CPU | CPU pico (%) | | | |
| 2 — Escrita | Tempo real (s) | | | |
| 2 — Escrita | Throughput (MB/s) | | | |
| 2 — Escrita | %iowait no host | | | |
| 3 — Leitura | Tempo real (s) | | | |
| 3 — Leitura | Throughput (MB/s) | | | |
| 3 — Leitura | %iowait no host | | | |
| 4 — Syscalls | Total de chamadas (ls) | | N/A | |
| 4 — Syscalls | Total de chamadas (cat) | | N/A | |

---

## 5. Análise e Discussão

### 5.1 Impacto do host por cargas executadas na VM

<!-- TODO: descrever correlação entre carga no guest e consumo observado no host -->

### 5.2 Comportamento do processo da VM durante os testes

<!-- TODO: CPU e memória do processo da VM (ex.: VBoxHeadless) durante cada experimento -->

### 5.3 Escrita no guest vs. atividade perceptível no host

<!-- TODO: analisar se a escrita via dd gerou atividade de I/O mensurável no host;
           discutir flush, fdatasync e o papel do cache do hipervisor -->

### 5.4 Diferença entre leitura e escrita

<!-- TODO: comparar throughput, iowait e latência; discutir cache de page do host -->

### 5.5 Dependência da camada de virtualização

<!-- TODO: discutir overhead do hipervisor; diferença entre Full Virtualization e
           Paravirtualização; impacto de VT-x / AMD-V -->

### 5.6 Evidências da atuação do hipervisor como intermediador

<!-- TODO: citar saídas específicas das ferramentas que evidenciam a interceptação
           de recursos pelo hipervisor (ex.: escrita no guest → arquivo .vdi cresce no host) -->

### 5.7 Limitações experimentais

<!-- TODO: listar interferências de outros processos, variabilidade do cache, limitações
           das ferramentas de medição, alocação dinâmica de disco, etc. -->

---

## 6. Conclusão

<!-- TODO: síntese dos achados; retomar o papel do hipervisor como intermediador;
           reflexão sobre como a camada de virtualização afeta desempenho e observabilidade -->

---

## Referências

<!-- TODO: adicionar referências utilizadas -->
