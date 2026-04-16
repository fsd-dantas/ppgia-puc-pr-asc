# Leitura Arquivo Multithread

Documento de apoio interno para entender e reproduzir a atividade.

**Importante:** a entrega solicitada pelo professor deve conter apenas o arquivo `ContadorCaracteres.java`.

## Enunciado resumido

Desenvolver um programa em Java que leia todos os arquivos `.txt` de um diretorio, conte as letras do alfabeto e imprima:

- a contagem consolidada de `A` ate `Z`;
- o tempo total gasto no processamento;
- uma configuracao de threads adequada ao computador usado.

O programa deve ser testado primeiro com a amostra de 100 arquivos e depois com a base de producao de 35.000 arquivos.

## Arquivos da atividade

| Caminho | Finalidade |
|---|---|
| `ContadorCaracteres.java` | Codigo fonte que deve ser entregue |
| `dados/amostra/` | Pasta local para extrair `amostra.zip` |
| `dados/todosArquivos/` | Pasta local para extrair `todosArquivos.zip` |
| `resultados/` | Saidas de execucao usadas no relatorio |
| `build/` | Saida local de compilacao, ignorada pelo Git |

As pastas `dados/`, `resultados/` e `build/` nao fazem parte da entrega.

## Dependencias

Nao ha bibliotecas externas, Maven ou Gradle. Basta ter um JDK instalado.

| Dependencia | Uso |
|---|---|
| JDK 21 | Compilar e executar o programa |
| `java.nio.file.Files` | Percorrer diretorios e arquivos `.txt` |
| `java.util.concurrent.ExecutorService` | Processar arquivos em paralelo |
| `java.text.Normalizer` | Normalizar letras acentuadas antes da contagem |

## Preparacao dos dados

Extraia os arquivos recebidos do professor nestas pastas:

```text
atividades/contador-caracteres/dados/amostra/
atividades/contador-caracteres/dados/todosArquivos/
```

O programa percorre subpastas recursivamente, entao nao ha problema se o `.zip` criar uma pasta interna.

## Compilacao

Dentro da pasta `atividades/contador-caracteres`:

```bash
javac -encoding UTF-8 ContadorCaracteres.java
```

Opcionalmente, para separar os `.class` gerados:

```bash
mkdir -p build
javac -encoding UTF-8 -d build ContadorCaracteres.java
```

No PowerShell:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
javac -encoding UTF-8 -d build ContadorCaracteres.java
```

## Execucao

Quando os `.class` forem gerados na propria pasta:

```bash
java ContadorCaracteres dados/amostra
java ContadorCaracteres dados/todosArquivos
```

Quando os `.class` forem gerados em `build/`:

```bash
java -cp build ContadorCaracteres dados/amostra
java -cp build ContadorCaracteres dados/todosArquivos
```

Tambem e possivel informar manualmente a quantidade de threads:

```bash
java -cp build ContadorCaracteres dados/todosArquivos 12
```

Nesta maquina, a melhor configuracao medida foi `16` threads. Sem informar o segundo argumento, o programa usa essa configuracao padrao.

## Analise de threads

Para comparar diferentes quantidades de threads:

```bash
java -cp build ContadorCaracteres --analisar dados/todosArquivos 1,2,4,6,8,12,16,24
```

O programa imprime o tempo de cada configuracao e indica a melhor configuracao naquela execucao.

Depois da analise com os arquivos reais, ajuste a constante abaixo no codigo, se necessario:

```java
private static final int MELHOR_CONFIGURACAO_THREADS = 16;
```

Nesta maquina, o valor final escolhido foi `16`, com base na media das repeticoes aquecidas registradas em [`resultados/`](resultados/).

## Criterios da contagem

- Conta apenas letras de `A` ate `Z`.
- Diferencas entre maiusculas e minusculas sao ignoradas.
- Letras acentuadas sao normalizadas: `á` conta como `A`, `ç` conta como `C`.
- Caracteres que nao sejam letras do alfabeto latino basico sao ignorados.
- Arquivos `.txt` sao lidos em UTF-8; caracteres invalidos sao substituidos durante a leitura.

## Exemplo de saida

```text
Diretorio: .../dados/amostra
Arquivos .txt processados: 100
Threads utilizadas: 12

A: 12345
B: 6789
...
Z: 321

Tempo de processamento: 0.842 s (842 ms)
```
