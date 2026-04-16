# Trabalho 02 - Problema Produtor-Consumidor

- **Disciplina:** Arquitetura de Sistemas Computacionais - PPGIA/PUC-PR
- **Autor:** Fernando Dantas
- **Modalidade:** Individual
- **Linguagem:** Java 21

Documento de apoio. Registra o enunciado, a implementação e o passo a passo de execução da atividade.

---

## 1. Objetivo

Implementar o problema clássico de produtor-consumidor em Java usando uma fila única, compartilhada entre múltiplas threads e com capacidade limitada.

O programa executa com:

- 03 threads produtoras;
- 02 threads consumidoras;
- fila compartilhada com capacidade máxima de 10 números;
- intervalo de produção `X` configurável;
- intervalo de consumo `Y` configurável.

---

## 2. Requisitos Atendidos

| Requisito | Implementação |
|---|---|
| Fila única compartilhada | `ArrayBlockingQueue<Integer>` compartilhada por todas as threads |
| Capacidade máxima de 10 números | Fila criada com capacidade fixa `10` |
| Produtor aguarda fila cheia | Uso de `fila.put(numero)` |
| Consumidor aguarda fila vazia | Uso de `fila.take()` |
| IDs únicos por thread | IDs globais `01` a `05` impressos nas operações |
| 03 produtores | `Produtor 01`, `Produtor 02`, `Produtor 03` |
| 02 consumidores | `Consumidor 04`, `Consumidor 05` |
| Valores `X` e `Y` configuráveis | Argumentos de linha de comando |

---

## 3. Estrutura

```text
produto-consumidor/
|-- dados/
|   |-- README.md
|   |-- cenario1-producao-rapida.txt
|   |-- cenario2-equilibrio.txt
|   `-- cenario3-consumo-rapido.txt
|-- README.md
|-- relatorio.md
`-- src/
    `-- main/
        `-- java/
            `-- br/
                `-- pucpr/
                    `-- asc/
                        `-- produtoconsumidor/
                            `-- Main.java
```

---

## 4. Dependências

O programa não usa bibliotecas externas, Maven ou Gradle. É necessário apenas um JDK instalado.

| Dependência | Uso |
|---|---|
| JDK 21 | Compilar e executar o programa Java |
| `java.util.concurrent.ArrayBlockingQueue` | Fila compartilhada com capacidade fixa e bloqueio automático |
| `java.util.concurrent.BlockingQueue` | Contrato de fila bloqueante usado por produtores e consumidores |
| `java.util.concurrent.ThreadLocalRandom` | Geração dos números aleatórios |
| `java.time.Duration` | Tratamento dos intervalos `X`, `Y` e duração opcional |

---

## 5. Argumentos

| Argumento | Significado | Obrigatório | Padrão |
|---|---|---|---|
| `X` | intervalo, em segundos, entre produções | não | `1` |
| `Y` | intervalo, em segundos, entre consumos | não | `2` |
| `duracao` | duração total da execução, em segundos | não | execução contínua |

O programa aceita valores decimais positivos. Sem o argumento `duracao`, ele executa continuamente até ser interrompido com `Ctrl+C`.

---

## 6. Passo a Passo de Execução

### 6.1 Acessar a pasta da atividade

Na raiz do repositório:

```bash
cd atividades/produto-consumidor
```

### 6.2 Compilar com JDK

Em Linux, macOS, WSL ou Git Bash:

```bash
mkdir -p build
javac -encoding UTF-8 -d build src/main/java/br/pucpr/asc/produtoconsumidor/Main.java
```

No PowerShell:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
javac -encoding UTF-8 -d build src\main\java\br\pucpr\asc\produtoconsumidor\Main.java
```

### 6.3 Executar cenário padrão

Executa com `X=1`, `Y=2` e duração de 15 segundos:

```bash
java -cp build br.pucpr.asc.produtoconsumidor.Main 1 2 15
```

### 6.4 Executar com produção mais rápida

Este cenário tende a encher a fila e bloquear produtores.

```bash
java -cp build br.pucpr.asc.produtoconsumidor.Main 0.5 2.0 30
```

### 6.5 Executar com consumo mais rápido

Este cenário tende a esvaziar a fila e bloquear consumidores.

```bash
java -cp build br.pucpr.asc.produtoconsumidor.Main 2.0 0.5 30
```

### 6.6 Executar continuamente

Sem o terceiro argumento, o programa continua executando até interrupção manual.

```bash
java -cp build br.pucpr.asc.produtoconsumidor.Main 1 2
```

---

## 7. Execução com mise

O `mise` é usado internamente neste repositório apenas para fixar a versão do JDK e simplificar os comandos. Ele não é requisito da atividade.

Na raiz do repositório:

```bash
mise run produto-consumidor:run -- 1 2 15
```

---

## 8. Exemplo de Saída

```text
Iniciando Produto Consumidor: produtores=3, consumidores=2, capacidade=10, X=1 s, Y=2 s, duracao=15
Produtor 01 - Inseriu 42 na fila
Produtor 02 - Inseriu 17 na fila
Consumidor 04 - Retirou 42 da fila
Produtor 03 - Inseriu 88 na fila
Consumidor 05 - Retirou 17 da fila
```

---

## 9. Dados de Execução

As saídas completas usadas no relatório estão armazenadas em [`dados/`](dados/).

| Arquivo | Cenário |
|---|---|
| [`cenario1-producao-rapida.txt`](dados/cenario1-producao-rapida.txt) | Produção mais rápida que consumo |
| [`cenario2-equilibrio.txt`](dados/cenario2-equilibrio.txt) | Desequilíbrio moderado entre produção e consumo |
| [`cenario3-consumo-rapido.txt`](dados/cenario3-consumo-rapido.txt) | Consumo mais rápido que produção |

Para regenerar os arquivos localmente, execute os cenários e redirecione a saída para a pasta `dados/`.

Exemplo em PowerShell:

```powershell
java -cp build br.pucpr.asc.produtoconsumidor.Main 0.5 2.0 30 | Tee-Object -FilePath dados\cenario1-producao-rapida.txt
```

---

## 10. Observações Técnicas

- A fila é compartilhada por todas as threads produtoras e consumidoras.
- `ArrayBlockingQueue.put()` bloqueia automaticamente quando a fila atinge a capacidade máxima.
- `ArrayBlockingQueue.take()` bloqueia automaticamente quando a fila está vazia.
- O construtor da fila usa política justa (`fair = true`), reduzindo a chance de starvation sob contenção.
- O encerramento com duração definida interrompe as threads ao final do tempo informado.

---

## 11. Relatório

A análise técnica e a discussão dos cenários estão registradas em [Problema Produtor-Consumidor com Fila Limitada](relatorio.md).
