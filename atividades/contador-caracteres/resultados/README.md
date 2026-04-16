# Resultados de Execucao

Esta pasta contem as saidas de console usadas para preencher o relatorio da atividade Leitura Arquivo Multithread.

| Arquivo | Conteudo |
|---|---|
| `amostra-1-thread.txt` | Validacao da amostra com 1 thread |
| `amostra-4-threads.txt` | Validacao da amostra com 4 threads |
| `producao-analise-threads.txt` | Primeira rodada de producao, com cache frio |
| `producao-analise-threads-repeticao2.txt` | Segunda rodada de producao, cache aquecido |
| `producao-analise-threads-repeticao3.txt` | Terceira rodada de producao, cache aquecido |

A escolha final da configuracao usa a media das repeticoes 2 e 3, por reduzirem o efeito de cache frio do sistema de arquivos.
