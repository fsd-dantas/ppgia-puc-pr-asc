# EDF — Earliest Deadline First: Análise de Trade-offs

**Disciplina:** Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR

---

## Conceito

Algoritmo de escalonamento de tempo real com prioridade dinâmica: a cada instante, a tarefa com o **prazo mais próximo** recebe a CPU. Preemptivo por natureza.

---

## Vantagens

| # | Vantagem | Detalhe |
|---|---|---|
| 1 | **Ótimo para uniprocessador** | Se algum algoritmo consegue escalonar um conjunto de tarefas sem perder prazos, EDF também consegue |
| 2 | **Utilização máxima de CPU** | Garante escalonabilidade com utilização U ≤ 100%, contra ~69,3% do Rate Monotonic (RM) |
| 3 | **Prioridade dinâmica** | Adapta-se automaticamente a conjuntos de tarefas com períodos variáveis ou tarefas esporádicas |
| 4 | **Condição de escalonabilidade simples** | Não exige análise por tarefa; basta verificar ∑(Ci/Di) ≤ 1 |

---

## Desvantagens

| # | Desvantagem | Detalhe |
|---|---|---|
| 1 | **Comportamento catastrófico em sobrecarga** | Quando U > 1, pode ocorrer o "efeito dominó": múltiplas tarefas perdem prazos simultaneamente, ao invés de degradação controlada |
| 2 | **Complexidade de implementação** | Exige reordenação dinâmica da fila de prontos a cada chegada de tarefa ou preempção |
| 3 | **Imprevisibilidade de jitter** | Tempos de resposta variáveis dificultam análise temporal em sistemas com restrições rígidas de jitter |
| 4 | **Não ótimo para multiprocessador** | A extensão para múltiplos processadores é NP-difícil; EDF global pode causar anomalias de escalonamento |
| 5 | **Baixa adoção em SOs convencionais** | A maioria dos kernels (Linux, Windows) usa prioridades fixas; EDF exige suporte explícito (ex.: `SCHED_DEADLINE` no Linux) |

---

## Comparação com Rate Monotonic (RM)

| Critério | EDF | RM |
|---|---|---|
| Tipo de prioridade | Dinâmica | Estática (por período) |
| Utilização máxima garantida | 100% | ~69,3% |
| Comportamento em sobrecarga | Imprevisível (efeito dominó) | Previsível (tarefas de menor prioridade perdem primeiro) |
| Complexidade de implementação | Alta | Baixa |
| Condição de escalonabilidade | ∑(Ci/Di) ≤ 1 | ∑(Ci/Ti) ≤ n(2^(1/n) − 1) |
| Suporte em SO | Limitado (`SCHED_DEADLINE`) | Amplo (mapeável em prioridades fixas) |

---

## Relevância sob Virtualização

Em ambiente virtualizado, o hipervisor interpõe uma camada de escalonamento entre o SO convidado e o hardware físico. Mesmo que o guest implemente EDF, o hipervisor pode preemptar a vCPU em qualquer momento, **violando os prazos do guest sem aviso**.

Isso torna garantias de tempo real sob hipervisores convencionais (KVM, VMware) essencialmente inválidas. Para cenários que exigem RT sob virtualização, são necessários hipervisores RT-aware, como:

| Hipervisor | Abordagem |
|---|---|
| **Jailhouse** | Particionamento estático de CPUs e memória; sem escalonamento compartilhado |
| **Xvisor** | Hipervisor tipo-1 com suporte a passthrough de interrupções |
| **ACRN** | Voltado a sistemas embarcados e automotivos; partições RT isoladas |

---

## Referências

- Liu, C. L.; Layland, J. W. *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment*. JACM, 1973.
- Buttazzo, G. *Hard Real-Time Computing Systems*. Springer, 3ª ed., 2011.
- Brandenburg, B. *Scheduling and Locking in Multiprocessor Real-Time Operating Systems*. PhD Thesis, UNC, 2011.
