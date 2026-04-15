# Contador de Caracteres Paralelo: Análise de Escalabilidade com a Lei de Amdahl

**Disciplina:** Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR  
**Autor:** [NOME]  
**Data:** [DATA]

---

## Resumo

Este trabalho implementa e analisa um contador de frequência de letras (A–Z) sobre um conjunto de arquivos de texto utilizando um pool de threads Java com `ExecutorService`. O objetivo principal é medir o speedup em função do número de threads e contrastar os resultados empíricos com o modelo teórico da Lei de Amdahl [[Amdahl67]](#ref-amdahl67). A implementação utiliza `AtomicInteger` como índice compartilhado para distribuição dinâmica de trabalho entre workers, evitando particionamento estático e minimizando o overhead de coordenação. Os experimentos cobrem configurações de 1 a 24 threads aplicadas a uma carga de 35.000 arquivos de texto.

---

## 1. Introdução

O processamento paralelo de grandes volumes de arquivos é um padrão recorrente em sistemas de análise de dados, indexação de conteúdo e processamento em lote. A questão central é: dado um conjunto de N arquivos e T threads disponíveis, qual a configuração ótima de threads e qual o speedup máximo atingível?

A Lei de Amdahl estabelece que, para um programa com fração serial `p` e paralela `(1-p)`, o speedup máximo com T threads é:

$$S(T) = \frac{1}{p + \frac{1-p}{T}}$$

Este modelo prediz que o speedup é limitado pela fração serial irredutível do programa, independentemente do número de threads adicionadas. Para carga predominantemente I/O-bound, a fração serial inclui tempo de abertura de arquivo, alocação de buffer e escrita do resultado final.

---

## 2. Metodologia

### 2.1 Descrição da implementação

A implementação utiliza um modelo de distribuição dinâmica de trabalho baseado em `AtomicInteger`:

```
Índice global: AtomicInteger idx = 0

Cada worker:
  enquanto (i = idx.getAndIncrement()) < N:
    processar arquivos[i]
    acumular contagem local
  somar contagem local ao resultado global
```

Esta abordagem elimina a necessidade de dividir o array de arquivos previamente, balanceando a carga naturalmente entre threads de velocidade heterogênea.

### 2.2 Hardware utilizado

| Atributo | Valor |
|---|---|
| Processador | [COMPLETAR: ex. Intel Core i7-3770 @ 3.40 GHz] |
| Núcleos físicos / lógicos | [COMPLETAR: ex. 4 / 8 (HT)] |
| RAM | [COMPLETAR: ex. 16 GB] |
| Armazenamento | [COMPLETAR: ex. HDD SATA] |
| JDK | 21.0.2 |

### 2.3 Conjuntos de dados

| Dataset | Arquivos | Tamanho estimado | Propósito |
|---|---|---|---|
| Amostra | 100 arquivos `.txt` | ~10 MB | Validação de correção |
| Produção | 35.000 arquivos `.txt` | [COMPLETAR] | Análise de escalabilidade |

### 2.4 Configurações de threads testadas

```bash
# Análise de escalabilidade (dataset de produção)
java ContadorCaracteres --analisar dados/todosArquivos 1,2,4,6,8,12,16,24
```

Configurações: T ∈ {1, 2, 4, 6, 8, 12, 16, 24}

---

## 3. Resultados

### 3.1 Validação de correção (dataset de amostra)

> **[COMPLETAR]** Executar com dataset de 100 arquivos e registrar:
> - Contagens por letra (A–Z)
> - Tempo de execução com T=1 e T=4
> - Confirmar que ambos produzem resultado idêntico

```
[SAÍDA DO PROGRAMA AQUI]
```

### 3.2 Análise de escalabilidade (dataset de produção)

**Tabela 1 — Speedup por configuração de threads**

| Threads (T) | Tempo (s) | Speedup S(T) = T₁/Tₜ | Eficiência E(T) = S(T)/T |
|---|---|---|---|
| 1  | [MEDIR] | 1,00 | 1,00 |
| 2  | [MEDIR] | [CALC] | [CALC] |
| 4  | [MEDIR] | [CALC] | [CALC] |
| 6  | [MEDIR] | [CALC] | [CALC] |
| 8  | [MEDIR] | [CALC] | [CALC] |
| 12 | [MEDIR] | [CALC] | [CALC] |
| 16 | [MEDIR] | [CALC] | [CALC] |
| 24 | [MEDIR] | [CALC] | [CALC] |

> **[COMPLETAR]** Identificar o ponto de inflexão onde o speedup começa a diminuir (diminishing returns). Anotar o número de threads com maior eficiência E(T) e o número de threads com maior speedup absoluto.

### 3.3 Ajuste da Lei de Amdahl

Dado o speedup máximo observado S_max em T_max threads, estimar a fração serial:

$$p = \frac{\frac{1}{S_{max}} - \frac{1}{T_{max}}}{1 - \frac{1}{T_{max}}}$$

> **[COMPLETAR]** Calcular p a partir dos dados medidos. Plotar a curva teórica S(T) = 1 / (p + (1-p)/T) sobre os pontos medidos.

---

## 4. Análise

### 4.1 Natureza da carga: I/O-bound vs. CPU-bound

A carga de contar caracteres em arquivos de texto é **mista**:

| Componente | Natureza | Paralelizável? |
|---|---|---|
| Leitura de arquivo (`FileReader`) | I/O-bound | Sim (disco serve múltiplas leituras simultâneas) |
| Normalização Unicode (`Normalizer.normalize`) | CPU-bound | Sim |
| Contagem de letras (loop A–Z) | CPU-bound | Sim |
| `AtomicInteger.getAndIncrement()` | Serial (contention) | Não |
| Soma final das contagens | Serial | Não |

Para T > número de núcleos lógicos, o overhead de troca de contexto e contenção no `AtomicInteger` começa a superar o ganho de paralelismo. O ponto ótimo teórico é T = número de threads lógicos disponíveis.

### 4.2 Comparação com a Lei de Gustafson

A Lei de Amdahl pressupõe tamanho de problema fixo. A Lei de Gustafson [[Gustafson88]](#ref-gustafson88) modela cenários onde o problema escala com os recursos:

$$S_{scaled}(T) = p + T \cdot (1-p)$$

> **[COMPLETAR]** Se o dataset de produção (35.000 arquivos) pode ser considerado "escalado" em relação ao dataset de amostra (100 arquivos), discutir qual modelo é mais adequado para este experimento.

### 4.3 Impacto do armazenamento

Para carga I/O-bound em HDD (latência de seek ~8 ms), threads adicionais além da saturação de I/O introduzem contenção sem ganho. Em SSD NVMe (latência ~0,1 ms), o paralelismo de I/O é mais efetivo.

> **[COMPLETAR]** Comentar o tipo de armazenamento utilizado e se o bottleneck observado é I/O ou CPU.

### 4.4 Custo de normalização Unicode

`java.text.Normalizer.normalize()` converte caracteres acentuados (á, é, ç, etc.) para forma NFD antes da contagem. Para grandes volumes de texto com alta densidade de acentos, este passo pode dominar o tempo de CPU.

> **[COMPLETAR]** Estimar a proporção de caracteres acentuados no corpus e discutir se remover a normalização alteraria o ponto ótimo de threads.

---

## 5. Conclusão

> **[COMPLETAR]** Síntese de 2–3 parágrafos. Estrutura sugerida:
>
> **Parágrafo 1 (Lei de Amdahl):** "Os resultados confirmaram o modelo de Amdahl com fração serial estimada em p ≈ X. O speedup máximo observado foi S = Y em T = Z threads, próximo ao limite teórico S_max = 1/p = W. Adicionar threads além de Z não produz ganho mensurável..."
>
> **Parágrafo 2 (bottleneck):** "A análise indica que a carga é predominantemente [I/O-bound / CPU-bound] para o hardware utilizado. O ponto de saturação em T = Z threads corresponde ao número de [núcleos lógicos / canais de I/O]..."
>
> **Parágrafo 3 (implicações de design):** "Para escalabilidade além dos limites de Amdahl, a alternativa seria adotar um modelo de redução paralela com acumuladores locais por thread (eliminando a contention no AtomicInteger) ou migrar para um pipeline de processamento assíncrono com `CompletableFuture`..."

---

## Referências

- <a name="ref-amdahl67"></a>[Amdahl67] AMDAHL, G. M. Validity of the Single Processor Approach to Achieving Large Scale Computing Capabilities. *Proceedings of AFIPS SJCC*, p. 483–485, 1967. DOI: 10.1145/1465482.1465560.
- <a name="ref-gustafson88"></a>[Gustafson88] GUSTAFSON, J. L. Reevaluating Amdahl's Law. *Communications of the ACM*, v. 31, n. 5, p. 532–533, 1988. DOI: 10.1145/42411.42415.
- <a name="ref-goetz06"></a>[Goetz06] GOETZ, B. et al. *Java Concurrency in Practice*. Addison-Wesley, 2006. ISBN 978-0-321-34960-6.

*BibTeX completo em [`literatura/referencias.bib`](../../literatura/referencias.bib).*
