# Problema Produtor-Consumidor com Fila Limitada

- **Disciplina:** Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR
- **Autor:** Fernando Dantas
- **Repositório Git:** [fsd-dantas/ppgia-puc-pr-asc](https://github.com/fsd-dantas/ppgia-puc-pr-asc)
- **Código-fonte:** [atividades/produto-consumidor](https://github.com/fsd-dantas/ppgia-puc-pr-asc/tree/master/atividades/produto-consumidor)
- **Data:** 15/04/2026

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
| Desequilíbrio moderado | 1.0 | 1.5 | 0.67 | Fila tende a saturar mais lentamente |
| Consumo mais rápido | 2.0 | 0.5 | 4.0 | Fila esvazia; consumidores bloqueiam |

Execução de cada cenário:

```bash
# Cenário 1 — produção rápida
mise run produto-consumidor:run -- 0.5 2.0 30

# Cenário 2 — desequilíbrio moderado
mise run produto-consumidor:run -- 1.0 1.5 30

# Cenário 3 — consumo rápido
mise run produto-consumidor:run -- 2.0 0.5 30
```

---

## 3. Resultados

As saídas completas de execução foram preservadas em [`dados/`](dados/) para permitir rastreabilidade dos resultados. As tabelas abaixo consolidam os eventos registrados nesses arquivos.

### 3.1 Cenário 1 — Produção mais rápida (X=0,5 s, Y=2,0 s)

Neste cenário, a produção nominal é muito superior ao consumo nominal. A fila limitada atinge a capacidade máxima rapidamente e, a partir desse ponto, os produtores passam a depender da retirada de itens pelos consumidores para continuar.

Trecho representativo da execução:

```text
Iniciando Produto Consumidor: produtores=3, consumidores=2, capacidade=10, X=0,500 s, Y=2 s, duracao=30
Produtor 01 - Inseriu 70 na fila
Consumidor 04 - Retirou 70 da fila
Produtor 02 - Inseriu 24 na fila
Produtor 03 - Inseriu 15 na fila
Consumidor 05 - Retirou 24 da fila
```

Resumo consolidado da execução de 30 segundos:

Arquivo completo: [`dados/cenario1-producao-rapida.txt`](dados/cenario1-producao-rapida.txt).

| Métrica | Valor observado |
|---|---:|
| Inserções totais | 40 |
| Retiradas totais | 30 |
| Saldo final estimado da fila | 10 itens |
| Produtor 01 | 13 inserções |
| Produtor 02 | 15 inserções |
| Produtor 03 | 12 inserções |
| Consumidor 04 | 15 retiradas |
| Consumidor 05 | 15 retiradas |

O saldo final igual a 10 confirma que a fila terminou cheia. Como a capacidade máxima é 10, isso indica que os produtores foram limitados pela capacidade do buffer durante boa parte da execução.

### 3.2 Cenário 2 — Desequilíbrio moderado (X=1,0 s, Y=1,5 s)

Embora este cenário reduza o desequilíbrio em relação ao anterior, a produção nominal ainda é maior que o consumo nominal. Portanto, a fila também tende a encher, mas de maneira menos abrupta.

Trecho representativo da execução:

```text
Iniciando Produto Consumidor: produtores=3, consumidores=2, capacidade=10, X=1 s, Y=1,500 s, duracao=30
Produtor 01 - Inseriu 30 na fila
Produtor 03 - Inseriu 14 na fila
Consumidor 05 - Retirou 30 da fila
Consumidor 04 - Retirou 14 da fila
Produtor 02 - Inseriu 87 na fila
```

Resumo consolidado da execução de 30 segundos:

Arquivo completo: [`dados/cenario2-equilibrio.txt`](dados/cenario2-equilibrio.txt).

| Métrica | Valor observado |
|---|---:|
| Inserções totais | 50 |
| Retiradas totais | 40 |
| Saldo final estimado da fila | 10 itens |
| Produtor 01 | 17 inserções |
| Produtor 02 | 16 inserções |
| Produtor 03 | 17 inserções |
| Consumidor 04 | 20 retiradas |
| Consumidor 05 | 20 retiradas |

O resultado mostra distribuição praticamente uniforme entre produtores e consumidores. A fila também terminou cheia, indicando que, mesmo com menor diferença de taxas, o sistema alcançou o limite do buffer dentro da janela de teste.

### 3.3 Cenário 3 — Consumo mais rápido (X=2,0 s, Y=0,5 s)

Neste cenário, o consumo nominal é maior que a produção nominal. Consequentemente, a fila permanece vazia durante grande parte da execução, e os consumidores bloqueiam em `take()` até que um produtor insira novo item.

Trecho representativo da execução:

```text
Iniciando Produto Consumidor: produtores=3, consumidores=2, capacidade=10, X=2 s, Y=0,500 s, duracao=30
Produtor 01 - Inseriu 20 na fila
Consumidor 04 - Retirou 20 da fila
Produtor 03 - Inseriu 50 na fila
Produtor 02 - Inseriu 24 na fila
Consumidor 05 - Retirou 50 da fila
```

Resumo consolidado da execução de 30 segundos:

Arquivo completo: [`dados/cenario3-consumo-rapido.txt`](dados/cenario3-consumo-rapido.txt).

| Métrica | Valor observado |
|---|---:|
| Inserções totais | 45 |
| Retiradas totais | 45 |
| Saldo final estimado da fila | 0 itens |
| Produtor 01 | 15 inserções |
| Produtor 02 | 15 inserções |
| Produtor 03 | 15 inserções |
| Consumidor 04 | 27 retiradas |
| Consumidor 05 | 18 retiradas |

O saldo final igual a 0 confirma que os consumidores acompanharam toda a produção disponível. A diferença entre os consumidores não caracteriza starvation, pois ambos realizaram retiradas durante a execução; ela reflete a competição normal entre threads e a política de escalonamento da JVM e do sistema operacional.

---

## 4. Análise

### 4.1 Correção via Java Memory Model

`ArrayBlockingQueue` garante a relação *happens-before* entre uma operação `put(e)` e o `take()` correspondente. Isso significa que qualquer estado de memória visível ao produtor no momento do `put` é garantidamente visível ao consumidor após o `take` [[Manson05]](#ref-manson05). Esta garantia é mais forte que a exclusão mútua simples: ela elimina a necessidade de `volatile` ou sincronização adicional nas variáveis produzidas.

### 4.2 Fairness e starvation

O `ArrayBlockingQueue` foi instanciado com política de fairness ativada:

```java
BlockingQueue<Integer> fila = new ArrayBlockingQueue<>(CAPACIDADE_FILA, true);
```

Com `fair = true`, o `ReentrantLock` interno tenta respeitar uma ordem de atendimento mais próxima de FIFO para threads bloqueadas. Isso reduz a possibilidade de *barging*, mas não transforma a execução em um revezamento perfeitamente alternado. A justiça vale para a aquisição do lock sob contenção; a ordem de execução ainda depende do escalonador da JVM, do sistema operacional e dos tempos de `sleep`.

Nos cenários 1 e 2, as contagens ficaram bem distribuídas:

| Cenário | Produtor 01 | Produtor 02 | Produtor 03 | Consumidor 04 | Consumidor 05 |
|---|---:|---:|---:|---:|---:|
| Produção rápida | 13 | 15 | 12 | 15 | 15 |
| Desequilíbrio moderado | 17 | 16 | 17 | 20 | 20 |
| Consumo rápido | 15 | 15 | 15 | 27 | 18 |

Não houve evidência de starvation. No terceiro cenário, o consumidor 04 retirou mais itens que o consumidor 05, mas o consumidor 05 também progrediu continuamente. Esse comportamento é aceitável para uma aplicação concorrente: fairness reduz assimetrias extremas, mas não garante igualdade aritmética entre threads.

### 4.3 Taxa efetiva vs. taxa configurada

A taxa efetiva de produção/consumo depende não só dos intervalos X e Y configurados, mas do bloqueio imposto pela fila. No Cenário 1:

- Taxa nominal de produção: 3 produtores × (1/X) = 3/0,5 = 6 items/s
- Taxa nominal de consumo: 2 consumidores × (1/Y) = 2/2,0 = 1 item/s
- Excesso: 5 items/s → fila satura em ~2 s

A execução não registra timestamps por evento, portanto o instante exato de saturação foi estimado pela taxa nominal e pelo saldo final da fila. O resultado é coerente com a previsão: ao final de 30 segundos, o Cenário 1 produziu 40 itens, consumiu 30 e terminou com 10 itens na fila, exatamente a capacidade máxima.

Taxas efetivas observadas:

| Cenário | Produção efetiva | Consumo efetivo | Interpretação |
|---|---:|---:|---|
| Produção rápida | 40/30 s = 1,33 item/s | 30/30 s = 1,00 item/s | Produção limitada pela fila cheia |
| Desequilíbrio moderado | 50/30 s = 1,67 item/s | 40/30 s = 1,33 item/s | Produção ainda excede consumo; fila termina cheia |
| Consumo rápido | 45/30 s = 1,50 item/s | 45/30 s = 1,50 item/s | Consumo limitado pela disponibilidade de itens |

Assim, a taxa configurada nos argumentos define apenas o ritmo máximo de cada thread. A taxa efetiva do sistema é determinada pela interação entre esses ritmos e a capacidade limitada da fila.

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

A implementação com `ArrayBlockingQueue` demonstrou ser correta nos três cenários testados. A fila compartilhada permaneceu limitada a 10 elementos, os produtores bloquearam quando não havia espaço disponível e os consumidores bloquearam quando não havia itens para retirar. A garantia *happens-before* do JMM assegura que os valores inseridos pelos produtores sejam observados corretamente pelos consumidores sem necessidade de `volatile`, `synchronized` explícito ou controle manual de semáforos.

Os resultados evidenciam o papel da fila como mecanismo de *backpressure*. Nos cenários em que a produção nominal superou o consumo nominal, a fila terminou cheia, com saldo final de 10 itens. No cenário em que o consumo nominal superou a produção nominal, a fila terminou vazia e as 45 inserções foram acompanhadas por 45 retiradas. Portanto, a taxa efetiva do sistema não é apenas função de X e Y, mas também da capacidade da fila e dos bloqueios impostos por `put` e `take`.

Do ponto de vista prático, `ArrayBlockingQueue` é adequada para esta atividade porque concentra, em uma abstração padrão da linguagem, os principais requisitos do problema: fila única compartilhada, capacidade fixa, bloqueio automático e segurança de memória. A opção por `fair = true` melhora a previsibilidade em situações de contenção, ainda que não garanta divisão perfeitamente igual de operações entre threads. Para aplicações reais, a escolha entre fairness e maior throughput deve ser avaliada conforme o objetivo do sistema.

---

## Referências

- <a name="ref-dijkstra68"></a>[Dijkstra68] DIJKSTRA, E. W. Cooperating Sequential Processes. In: GENUYS, F. (ed.). *Programming Languages*. Academic Press, 1968. p. 43–112.
- <a name="ref-goetz06"></a>[Goetz06] GOETZ, B. et al. *Java Concurrency in Practice*. Addison-Wesley, 2006. ISBN 978-0-321-34960-6.
- <a name="ref-manson05"></a>[Manson05] MANSON, J.; PUGH, W.; ADVE, S. V. The Java Memory Model. *ACM SIGPLAN Notices*, v. 40, n. 1, p. 378–391, 2005.

*BibTeX completo em [`literatura/referencias.bib`](../../literatura/referencias.bib).*
