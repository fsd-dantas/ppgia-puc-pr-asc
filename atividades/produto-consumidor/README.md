# Produto Consumidor

**Disciplina:** Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR

## Objetivo

Implementar o problema clássico de produtor-consumidor em Java usando uma fila única compartilhada entre múltiplas threads.

## Requisitos atendidos

| Requisito | Implementação |
|---|---|
| Fila única compartilhada | `ArrayBlockingQueue<Integer>` compartilhada por todas as threads |
| Capacidade máxima 10 | Fila criada com capacidade fixa `10` |
| Produtor aguarda fila cheia | Uso de `fila.put(numero)` |
| Consumidor aguarda fila vazia | Uso de `fila.take()` |
| IDs únicos por thread | IDs globais `01` a `05` impressos nas operações |
| 03 produtores | `Produtor 01`, `Produtor 02`, `Produtor 03` |
| 02 consumidores | `Consumidor 04`, `Consumidor 05` |
| X e Y configuráveis | Argumentos de linha de comando |

## Dependências

O programa não usa bibliotecas externas, Maven ou Gradle. É necessário apenas um JDK instalado.

| Dependência | Uso |
|---|---|
| JDK 21 | Compilar e executar o programa Java |
| `java.util.concurrent.ArrayBlockingQueue` | Fila compartilhada com capacidade fixa e bloqueio automático |
| `java.util.concurrent.BlockingQueue` | Contrato de fila bloqueante usado por produtores e consumidores |
| `java.util.concurrent.ThreadLocalRandom` | Geração dos números aleatórios |
| `java.time.Duration` | Tratamento dos intervalos `X`, `Y` e duração opcional |

## Argumentos

| Argumento | Significado | Obrigatório | Padrão |
|---|---|---|---|
| `X` | intervalo, em segundos, entre produções | não | `1` |
| `Y` | intervalo, em segundos, entre consumos | não | `2` |
| `duracao` | duração total da execução, em segundos | não | execução contínua |

O programa aceita valores decimais positivos. Sem o argumento `duracao`, ele executa continuamente até ser interrompido com `Ctrl+C`.

## Execução direta

Dentro da pasta `atividades/produto-consumidor`, compile e execute com o JDK:

```bash
mkdir -p build
javac -encoding UTF-8 -d build src/main/java/br/pucpr/asc/produtoconsumidor/Main.java
java -cp build br.pucpr.asc.produtoconsumidor.Main 1 2 15
```

No PowerShell, caso a pasta `build` ainda não exista:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
javac -encoding UTF-8 -d build src\main\java\br\pucpr\asc\produtoconsumidor\Main.java
java -cp build br.pucpr.asc.produtoconsumidor.Main 1 2 15
```

Exemplo com `X=0.5`, `Y=1.25` e duração de 10 segundos:

```bash
java -cp build br.pucpr.asc.produtoconsumidor.Main 0.5 1.25 10
```

## Execução com mise

O `mise` é usado internamente neste repositório apenas para fixar a versão do JDK e simplificar os comandos.

Na raiz do repositório:

```bash
mise run produto-consumidor:run -- 1 2 15
```

## Exemplo de saída

```text
Produtor 02 - Inseriu 14 na fila
Consumidor 04 - Retirou 14 da fila
```
