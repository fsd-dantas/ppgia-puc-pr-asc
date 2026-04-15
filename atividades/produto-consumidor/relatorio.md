# Problema Produtor-Consumidor com Fila Limitada

**Disciplina:** Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR  
**Autor:** [NOME]  
**Data:** [DATA]

---

## Resumo

Este trabalho implementa e analisa o problema clássico do Produtor-Consumidor com fila limitada utilizando as primitivas de concorrência da biblioteca padrão Java 21. São investigadas as garantias de correção oferecidas por `ArrayBlockingQueue` à luz do Java Memory Model (JMM), as condições de starvation sob diferentes configurações de taxa produtor/consumidor, e a adequação do padrão de bloqueio implícito (`put`/`take`) em comparação com semáforos explícitos. A implementação emprega 3 produtores e 2 consumidores com fila de capacidade 10, permitindo verificar empiricamente o comportamento do sistema sob desequilíbrio de taxas.

---

## 1. Introdução

O problema Produtor-Consumidor, formulado originalmente por Dijkstra [[Dijkstra68]](#ref-dijkstra68), é a abstração canônica para sistemas que envolvem produção e consumo assíncrono de dados através de um buffer compartilhado. Sua correta implementação requer três garantias simultâneas:

1. **Exclusão mútua:** acesso ao buffer é mutuamente exclusivo;
2. **Sincronização por condição:** produtor espera quando buffer cheio; consumidor espera quando vazio;
3. **Ausência de starvation:** nenhuma thread fica bloqueada indefinidamente sob carga finita.

Em Java, `ArrayBlockingQueue` encapsula essas três garantias por meio de um par de `Condition` sobre um `ReentrantLock`, conforme especificado pelo JMM [[Manson05]](#ref-manson05). Este trabalho explora em que condições essas garantias se mantêm e onde emergem comportamentos indesejados (starvation, inversão de taxa efetiva).

---

## 2. Metodologia

### 2.1 Configuração do experimento

| Parâmetro | Valor |
|---|---|
| Produtores | 3 (IDs 01–03) |
| Consumidores | 2 (IDs 04–05) |
| Capacidade da fila | 10 itens |
| Itens produzidos | Inteiros aleatórios ∈ [0, 99] |
| Intervalo de produção (X) | Configurável via argumento |
| Intervalo de consumo (Y) | Configurável via argumento |

### 2.2 Primitivas utilizadas

| Primitiva | Semântica | Garantia do JMM |
|---|---|---|
| `ArrayBlockingQueue.put(e)` | Bloqueia se fila cheia (capacidade = 10) | Happens-before entre `put` e `take` correspondente |
| `ArrayBlockingQueue.take()` | Bloqueia se fila vazia | Visibilidade garantida ao consumidor |
| `Thread.sleep(Duration)` | Simula tempo de produção/consumo | Não implica sincronização — apenas suspensão |
| `Runtime.addShutdownHook` | Encerramento limpo via Ctrl+C | Interrompe threads via `Thread.interrupt()` |

### 2.3 Cenários de teste

| Cenário | X (s) | Y (s) | Razão X/Y | Comportamento esperado |
|---|---|---|---|---|
| Produção mais rápida | 0.5 | 2.0 | 0.25 | Fila satura; produtores bloqueiam |
| Equilíbrio aproximado | 1.0 | 1.5 | 0.67 | Fila oscila; sem bloqueio prolongado |
| Consumo mais rápido | 2.0 | 0.5 | 4.0 | Fila esvazia; consumidores bloqueiam |

Execução de cada cenário:

```bash
# Cenário 1 — produção rápida
mise run produto-consumidor:run -- 0.5 2.0 30

# Cenário 2 — equilíbrio
mise run produto-consumidor:run -- 1.0 1.5 30

# Cenário 3 — consumo rápido
mise run produto-consumidor:run -- 2.0 0.5 30
```

---

## 3. Resultados

### 3.1 Cenário 1 — Produção mais rápida (X=0,5 s, Y=2,0 s)

> **[COMPLETAR]** Colar saída do programa. Observar:
> - Após quantos itens a fila satura e produtores bloqueiam;
> - Quais produtores ficam mais bloqueados (possível unfairness);
> - Contagem total de operações por thread ao final.

```
[SAÍDA DO PROGRAMA AQUI]
```

### 3.2 Cenário 2 — Equilíbrio (X=1,0 s, Y=1,5 s)

> **[COMPLETAR]** Observar flutuação do tamanho efetivo da fila.

```
[SAÍDA DO PROGRAMA AQUI]
```

### 3.3 Cenário 3 — Consumo mais rápido (X=2,0 s, Y=0,5 s)

> **[COMPLETAR]** Observar bloqueio dos consumidores quando fila esvazia.

```
[SAÍDA DO PROGRAMA AQUI]
```

---

## 4. Análise

### 4.1 Correção via Java Memory Model

`ArrayBlockingQueue` garante a relação *happens-before* entre uma operação `put(e)` e o `take()` correspondente. Isso significa que qualquer estado de memória visível ao produtor no momento do `put` é garantidamente visível ao consumidor após o `take` [[Manson05]](#ref-manson05). Esta garantia é mais forte que a exclusão mútua simples: ela elimina a necessidade de `volatile` ou sincronização adicional nas variáveis produzidas.

### 4.2 Fairness e starvation

O `ArrayBlockingQueue` utiliza `ReentrantLock` com política de fairness *não* ativada por padrão (`fair = false`). Isso significa que, sob contenção, a ordem de desbloqueio das threads não é FIFO — threads que acabam de ser desbloqueadas podem "barging" (re-adquirir o lock antes de threads que esperavam mais tempo). Em cenários de alta contenção:

> **[COMPLETAR]** Verificar empiricamente se algum produtor ou consumidor demonstra contagem de operações significativamente menor que os demais. Discutir se ativar `fair = true` no construtor de `ArrayBlockingQueue` altera o comportamento observado.

### 4.3 Taxa efetiva vs. taxa configurada

A taxa efetiva de produção/consumo depende não só dos intervalos X e Y configurados, mas do bloqueio imposto pela fila. No Cenário 1:

- Taxa nominal de produção: 3 produtores × (1/X) = 3/0,5 = 6 items/s
- Taxa nominal de consumo: 2 consumidores × (1/Y) = 2/2,0 = 1 item/s
- Excesso: 5 items/s → fila satura em ~2 s

> **[COMPLETAR]** Medir tempo até primeiro bloqueio e comparar com a previsão acima.

### 4.4 Comparação com semáforos explícitos

A implementação com `ArrayBlockingQueue` abstrai os dois semáforos clássicos (`emptySlots`, `fullSlots`) da solução de Dijkstra. As vantagens práticas são:

| Aspecto | Semáforos explícitos | ArrayBlockingQueue |
|---|---|---|
| Complexidade do código | Alta (gerenciamento manual) | Baixa (abstração completa) |
| Risco de deadlock | Presente (ordem de acquire) | Eliminado pela implementação |
| Flexibilidade | Alta (políticas customizáveis) | Moderada (configurações limitadas) |
| Fairness configurável | Dependente da implementação | `fair` como parâmetro do construtor |

---

## 5. Conclusão

> **[COMPLETAR]** Síntese de 2-3 parágrafos. Estrutura sugerida:
>
> **Parágrafo 1 (correção):** "A implementação com `ArrayBlockingQueue` demonstrou ser correta nos três cenários testados. A garantia *happens-before* do JMM assegurou visibilidade de memória sem sincronização adicional..."
>
> **Parágrafo 2 (comportamento sob desequilíbrio):** "Quando a taxa de produção excede a de consumo em X vezes, a fila satura em aproximadamente Y segundos. O comportamento de bloqueio de produtores é controlado, mas a política de fairness não-FIFO introduz..."
>
> **Parágrafo 3 (implicações práticas):** "Para sistemas de alto throughput onde fairness é crítica, é recomendável avaliar `fair = true` ou estruturas concorrentes de fila não-bloqueante (`ConcurrentLinkedQueue`). Para sistemas onde a semântica de backpressure (produtor para quando buffer cheio) é desejável — como pipelines de processamento de dados — `ArrayBlockingQueue` com `put` bloqueante é a abstração ideal..."

---

## Referências

- <a name="ref-dijkstra68"></a>[Dijkstra68] DIJKSTRA, E. W. Cooperating Sequential Processes. In: GENUYS, F. (ed.). *Programming Languages*. Academic Press, 1968. p. 43–112.
- <a name="ref-goetz06"></a>[Goetz06] GOETZ, B. et al. *Java Concurrency in Practice*. Addison-Wesley, 2006. ISBN 978-0-321-34960-6.
- <a name="ref-manson05"></a>[Manson05] MANSON, J.; PUGH, W.; ADVE, S. V. The Java Memory Model. *ACM SIGPLAN Notices*, v. 40, n. 1, p. 378–391, 2005.

*BibTeX completo em [`literatura/referencias.bib`](../../literatura/referencias.bib).*
