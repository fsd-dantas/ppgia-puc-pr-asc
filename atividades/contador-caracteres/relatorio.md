# Leitura Arquivo Multithread

- **Disciplina:** Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR
- **Autor:** Fernando Dantas
- **Data:** 15/04/2026
- **Status:** Resultados quantitativos coletados.

---

## Resumo

Este relatório documenta a atividade **Leitura Arquivo Multithread**, que implementa e analisa um contador de frequência de letras (A–Z) sobre um conjunto de arquivos de texto utilizando um pool de threads Java com `ExecutorService`. O objetivo principal é medir o speedup em função do número de threads e contrastar os resultados empíricos com o modelo teórico da Lei de Amdahl [[Amdahl67]](#ref-amdahl67). A implementação utiliza `AtomicInteger` como índice compartilhado para distribuição dinâmica de trabalho entre workers, evitando particionamento estático e minimizando o overhead de coordenação. Os experimentos cobrem configurações de 1 a 24 threads aplicadas a uma carga real de 34.055 arquivos de texto.

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
| Processador | 13th Gen Intel(R) Core(TM) i7-1355U |
| Núcleos físicos / lógicos | 10 / 12 (JVM reportou 12 processadores lógicos) |
| RAM | 15,69 GB |
| Armazenamento | Unidade local `D:`; tipo físico não identificado pelo ambiente sem CIM |
| Sistema operacional | Microsoft Windows 10.0.26200 |
| JDK | OpenJDK 21.0.2 |

### 2.3 Conjuntos de dados

| Dataset | Arquivos | Tamanho estimado | Propósito |
|---|---|---|---|
| Amostra | 100 arquivos `.txt` | 0,21 MB | Validação de correção |
| Produção | 34.055 arquivos `.txt` | 74,89 MB | Análise de escalabilidade |

O enunciado informa uma base de produção com 35.000 arquivos. A extração local e a inspeção do arquivo `todosArquivos.zip` retornaram 34.055 arquivos `.txt`; por isso, as medições deste relatório usam a quantidade efetivamente encontrada.

### 2.4 Configurações de threads testadas

```bash
# Análise de escalabilidade (dataset de produção)
java ContadorCaracteres --analisar dados/todosArquivos 1,2,4,6,8,12,16,24
```

Configurações: T ∈ {1, 2, 4, 6, 8, 12, 16, 24}

---

## 3. Resultados

As saídas completas de console foram preservadas em [`resultados/`](resultados/), incluindo a validação da amostra e as três rodadas de análise da base de produção.

### 3.1 Validação de correção (dataset de amostra)

Foram executadas duas leituras da amostra: uma com 1 thread e outra com 4 threads. As contagens por letra foram idênticas, confirmando que a paralelização não alterou o resultado.

| Configuração | Arquivos | Tempo | Resultado |
|---|---:|---:|---|
| 1 thread | 100 | 0,144 s | Contagem de referência |
| 4 threads | 100 | 0,050 s | Contagem idêntica |

Contagem consolidada da amostra:

| Letra | Contagem | Letra | Contagem | Letra | Contagem | Letra | Contagem |
|---|---:|---|---:|---|---:|---|---:|
| A | 21.246 | H | 1.313 | O | 18.235 | V | 2.108 |
| B | 2.130 | I | 12.749 | P | 5.602 | W | 78 |
| C | 6.504 | J | 623 | Q | 1.470 | X | 472 |
| D | 9.155 | K | 65 | R | 12.231 | Y | 62 |
| E | 19.566 | L | 5.343 | S | 12.599 | Z | 626 |
| F | 1.923 | M | 6.893 | T | 8.331 |  |  |
| G | 2.361 | N | 8.709 | U | 6.363 |  |  |

### 3.2 Análise de escalabilidade (dataset de produção)

**Tabela 1 — Speedup por configuração de threads**

| Threads (T) | Tempo (s) | Speedup S(T) = T₁/Tₜ | Eficiência E(T) = S(T)/T |
|---|---|---|---|
| 1  | 4,33 | 1,00 | 1,00 |
| 2  | 2,44 | 1,78 | 0,89 |
| 4  | 1,57 | 2,76 | 0,69 |
| 6  | 1,30 | 3,35 | 0,56 |
| 8  | 1,05 | 4,11 | 0,51 |
| 12 | 0,96 | 4,51 | 0,38 |
| 16 | 0,81 | 5,37 | 0,34 |
| 24 | 0,95 | 4,56 | 0,19 |

Os valores da tabela usam a média das duas repetições aquecidas (`repeticao2` e `repeticao3`) para reduzir o efeito de cache frio do sistema de arquivos. A primeira execução completa foi preservada, mas não usada como base principal porque o tempo com 1 thread foi significativamente maior (11,332 s), indicando interferência de cache frio.

O maior speedup absoluto ocorreu com **16 threads**. A maior eficiência ocorreu com **2 threads**, mas essa configuração não minimiza o tempo total. Para submissão do código, a melhor configuração escolhida para esta máquina foi:

```java
private static final int MELHOR_CONFIGURACAO_THREADS = 16;
```

### 3.3 Ajuste da Lei de Amdahl

Dado o speedup máximo observado S_max em T_max threads, estimar a fração serial:

$$p = \frac{\frac{1}{S_{max}} - \frac{1}{T_{max}}}{1 - \frac{1}{T_{max}}}$$

Com `S_max = 5,37` em `T_max = 16`, a fração serial estimada é:

$$p = \frac{\frac{1}{5{,}37} - \frac{1}{16}}{1 - \frac{1}{16}} \approx 0{,}1319$$

Assim, a fração serial estimada é de aproximadamente **13,19%**. O limite teórico de speedup para essa fração serial seria:

$$S_{limite} = \frac{1}{p} \approx 7{,}58$$

O resultado empírico fica abaixo desse limite, como esperado, devido a custos práticos de leitura de arquivos, criação e agendamento de threads, contenção no índice compartilhado e pressão de cache.

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

O comportamento observado confirma que a carga é mista. A execução melhora fortemente entre 1 e 8 threads, continua melhorando até 16 threads, e perde desempenho em 24 threads. Mesmo com 12 processadores lógicos reportados pela JVM, 16 threads apresentou melhor tempo médio, provavelmente por sobrepor esperas de I/O e processamento de normalização/contagem.

A partir de 24 threads, o overhead de escalonamento, contenção no `AtomicInteger` e concorrência por cache/memória passam a superar o ganho adicional de paralelismo.

### 4.2 Comparação com a Lei de Gustafson

A Lei de Amdahl pressupõe tamanho de problema fixo. A Lei de Gustafson [[Gustafson88]](#ref-gustafson88) modela cenários onde o problema escala com os recursos:

$$S_{scaled}(T) = p + T \cdot (1-p)$$

Neste experimento, a Lei de Amdahl é o modelo mais adequado, pois o tamanho do problema é fixo em cada rodada: o mesmo conjunto de 34.055 arquivos é processado com diferentes quantidades de threads. A Lei de Gustafson seria mais apropriada se o tamanho do dataset aumentasse proporcionalmente ao número de threads disponíveis.

Ainda assim, a comparação entre a amostra de 100 arquivos e a produção de 34.055 arquivos mostra que a granularidade do problema influencia a utilidade do paralelismo. No dataset pequeno, a sobrecarga de criação de threads e coordenação é proporcionalmente maior; no dataset grande, há trabalho suficiente para amortizar esse custo.

### 4.3 Impacto do armazenamento

Para carga I/O-bound em HDD (latência de seek ~8 ms), threads adicionais além da saturação de I/O introduzem contenção sem ganho. Em SSD NVMe (latência ~0,1 ms), o paralelismo de I/O é mais efetivo.

O tipo físico do armazenamento não pôde ser identificado no ambiente porque as consultas CIM estavam indisponíveis. Ainda assim, os resultados mostram um efeito claro de cache: a primeira rodada teve tempo de 11,332 s com 1 thread, enquanto as rodadas subsequentes aquecidas ficaram em torno de 4,33 s com 1 thread.

Isso indica que a primeira execução foi mais influenciada por I/O. Nas repetições aquecidas, a carga passa a se comportar mais como CPU/memória, dominada pela normalização Unicode, contagem de caracteres e coordenação entre workers.

### 4.4 Custo de normalização Unicode

`java.text.Normalizer.normalize()` converte caracteres acentuados (á, é, ç, etc.) para forma NFD antes da contagem. Para grandes volumes de texto com alta densidade de acentos, este passo pode dominar o tempo de CPU.

Foi realizada uma varredura por caracteres acentuados comuns em português (`á`, `é`, `í`, `ó`, `ú`, `ã`, `õ`, `ç` e variações maiúsculas). Não foram encontradas ocorrências na base de produção. A contagem normalizada total foi de 58.901.399 letras, com 0 letras acentuadas detectadas nessa varredura.

Portanto, para este corpus específico, a normalização Unicode não parece ser um fator dominante. Ela permanece no código por robustez, garantindo que arquivos com acentos sejam contabilizados corretamente, mas sua remoção provavelmente não alteraria o ponto ótimo de threads nesta base.

---

## 5. Conclusão

A implementação atendeu ao objetivo da atividade ao processar recursivamente arquivos `.txt`, consolidar a contagem de letras de `A` a `Z` e medir o tempo de processamento. A validação com a amostra de 100 arquivos confirmou que execuções com 1 e 4 threads geram exatamente a mesma contagem, indicando que a paralelização não compromete a correção do resultado.

Na base de produção, com 34.055 arquivos `.txt`, a melhor configuração observada foi **16 threads**, com tempo médio de **0,81 s** nas repetições aquecidas. O speedup máximo estimado foi **5,37**, com fração serial aproximada de **13,19%** pela Lei de Amdahl. Embora a máquina reporte 12 processadores lógicos, a melhor configuração acima desse valor sugere que há ganho em sobrepor leitura, normalização e contagem até certo ponto.

O experimento também evidencia os limites práticos da paralelização: 24 threads reduziu a eficiência e aumentou o tempo em relação a 16 threads. Assim, adicionar threads indefinidamente não melhora o desempenho; após o ponto ótimo, o custo de escalonamento, coordenação e pressão de memória supera o benefício do paralelismo adicional.

---

## Referências

- <a name="ref-amdahl67"></a>[Amdahl67] AMDAHL, G. M. Validity of the Single Processor Approach to Achieving Large Scale Computing Capabilities. *Proceedings of AFIPS SJCC*, p. 483–485, 1967. DOI: 10.1145/1465482.1465560.
- <a name="ref-gustafson88"></a>[Gustafson88] GUSTAFSON, J. L. Reevaluating Amdahl's Law. *Communications of the ACM*, v. 31, n. 5, p. 532–533, 1988. DOI: 10.1145/42411.42415.
- <a name="ref-goetz06"></a>[Goetz06] GOETZ, B. et al. *Java Concurrency in Practice*. Addison-Wesley, 2006. ISBN 978-0-321-34960-6.

*BibTeX completo em [`literatura/referencias.bib`](../../literatura/referencias.bib).*
