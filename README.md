# Arquitetura de Sistemas Computacionais

- **Programa:** PPGIA - Pós-Graduação em Informática Aplicada
- **Instituição:** Pontifícia Universidade Católica do Paraná - PUC-PR
- **Repositório:** [github.com/fsd-dantas/ppgia-puc-pr-asc](https://github.com/fsd-dantas/ppgia-puc-pr-asc)
- **Propósito:** referência acadêmica pública, reprodutível, sem a utilização de dados sensíveis de projeto

---

## Resumo

Este repositório organiza os artefatos experimentais da disciplina **Arquitetura de Sistemas Computacionais** do programa de Mestrado PPGIA/PUC-PR. Os trabalhos exploram virtualização e hipervisores, sincronização entre threads, leitura paralela de arquivos, análise de desempenho, determinismo em sistemas operacionais de tempo real (RTOS) e protocolos de roteamento para redes de baixa potência e com perdas.

O objetivo é manter uma base aberta, didática e academicamente rastreável, com código-fonte, instruções de execução, protocolos experimentais e relatórios técnicos quando aplicável. O material pode ser referenciado por projetos privados de pesquisa aplicada, desde que dados, ambientes, resultados, topologias e informações sensíveis permaneçam fora deste repositório público.

---

## Trabalhos

| #   | Título                                                                                                        | Tema central                                     | Stack de programação                                    | Status         |
| --- | ------------------------------------------------------------------------------------------------------------- | ------------------------------------------------ | ------------------------------------------------------- | -------------- |
| 01  | [Análise do Hypervisor e do Impacto da Máquina Virtual sobre o Host](atividades/hypervisor-vm-host/README.md) | Virtualização, KVM, observabilidade de recursos  | KVM/QEMU, libvirt, `pidstat`, `strace`, `iostat`        | Entregue 15/04 |
| 02  | [Problema Produtor-Consumidor](atividades/produto-consumidor/README.md)                                       | Concorrência, sincronização e fila bloqueante    | Java 21, `ArrayBlockingQueue`                           | Entregue 15/04 |
| 03  | [Leitura Arquivo Multithread](atividades/contador-caracteres/README.md)                                       | Paralelismo de dados e processamento de arquivos | Java 21, `ExecutorService`, `AtomicInteger`             | Entregue 15/04 |
| 04  | [RTOS: Determinismo e Inversão de Prioridade](atividades/rtos-determinismo/README.md)                         | Escalonamento, jitter, prioridade e RTOS         | C, pthreads, `SCHED_FIFO`, Zephyr, QEMU                 | Entregue 15/04 |
| 05  | [Estudo sobre Protocolo de Roteamento RPL](atividades/protocolo-rpl/README.md)                                | LLN, IoT, RPL, resiliência e roteamento          | Revisão bibliográfica, LaTeX, Overleaf, IEEE Conference | Em andamento   |

---

## Reprodutibilidade

### Princípios

- Os comandos manuais aparecem primeiro; automações locais são opcionais.
- Dados brutos grandes ou fornecidos pela disciplina estão fora do repositório.
- Resultados derivados registram ambiente, versão das ferramentas e parâmetros usados, mas informações sensíveis como IPs, senhas, localização geográficas, etc, foram omitidos dos relatórios.
- Scripts e fontes devem poder ser executados a partir da pasta da própria atividade.
- Conteúdo público deve permanecer genérico, acadêmico e sem dados operacionais sensíveis.

### Pré-requisitos

| Dependência | Versão recomendada | Uso |
|---|---|---|
| Java JDK | 21+ | Trabalhos 02 e 03 |
| Python | 3.11+ | Scripts de análise e gráficos do Trabalho 04 |
| GCC + pthreads | sistema | Trabalho 04 - ambiente Linux/WSL |
| Zephyr SDK + `west` | 3.6+ | Trabalho 04 - ambiente RTOS |
| QEMU | 8.x+ | Trabalho 04 - emulação `qemu_x86` |
| KVM/QEMU + libvirt | sistema | Trabalho 01 - virtualização |
| LaTeX/Overleaf | IEEE Conference | Trabalho 05 - artigo científico |

### Detalhes de Execução dos Trabalhos

**Trabalho 01 - Análise do Hipervisor e Impacto da VM sobre o Host**

Experimento baseado em ambiente KVM/QEMU com VM previamente configurada. O passo a passo de preparação está em [Setup do Ambiente - Registro de Procedimentos](atividades/hypervisor-vm-host/setup.md), e o protocolo de execução e monitoramento está em [Relatório Técnico - Análise do Hipervisor e do Impacto da Máquina Virtual sobre o Host](atividades/hypervisor-vm-host/relatorio.md).

**Trabalho 02 - Produtor-Consumidor**

O passo a passo completo está em [Trabalho 02 - Problema Produtor-Consumidor](atividades/produto-consumidor/README.md), e a análise técnica está em [Problema Produtor-Consumidor com Fila Limitada](atividades/produto-consumidor/relatorio.md).

Exemplo rápido com `X=1s`, `Y=2s` e duração de 15 segundos:

```bash
cd atividades/produto-consumidor
mkdir -p build
javac -encoding UTF-8 -d build src/main/java/br/pucpr/asc/produtoconsumidor/Main.java
java -cp build br.pucpr.asc.produtoconsumidor.Main 1 2 15
```

**Trabalho 03 - Leitura Arquivo Multithread**

Antes da execução, extraia a base local em `dados/amostra/` ou `dados/todosArquivos/`.

```bash
cd atividades/contador-caracteres
javac -encoding UTF-8 ContadorCaracteres.java
java ContadorCaracteres dados/todosArquivos
```

Para comparar configurações de threads:

```bash
java ContadorCaracteres --analisar dados/todosArquivos 1,2,4,6,8,12,16,24
```

**Trabalho 04 - Jitter no Linux e geração de gráficos**

Alguns experimentos usam prioridade real-time e podem exigir `root` ou capacidades equivalentes.

```bash
cd atividades/rtos-determinismo/src/linux
gcc -O2 -Wall -o jitter jitter.c -lm -lpthread
sudo ./jitter

cd ../../..
python -m pip install numpy matplotlib pandas
python scripts/plot_jitter.py
```

> **Atalho local com mise:** este repositório inclui um [`mise.toml`](mise.toml) com tarefas de apoio para desenvolvimento. O `mise` não é requisito para avaliar ou executar as atividades; ele é uma ferramenta para ambiente de desenvolvimento para organizar diferentes *stacks* de programação e reduz repetição de comandos em ambiente local.

---
## Política de Dados

Este repositório contém apenas material público e reproduzível.

| Tipo de conteúdo                                      | Versionado | Observação                                               |
| ----------------------------------------------------- | ---------- | -------------------------------------------------------- |
| Código-fonte didático                                 | Sim        | Java, C, scripts e configurações necessárias             |
| READMEs, relatórios e referências                     | Sim        | Desde que não exponham dados sensíveis                   |
| Dados brutos grandes                                  | Não        | Manter em `dados/` local, ignorado pelo Git              |
| Resultados derivados pequenos                         | Depende    | Versionar apenas se forem anônimos e reprodutíveis       |
| Topologias, ativos, credenciais ou dados operacionais | Não        | Devem permanecer em repositórios privados apropriados    |
| Artefatos de build                                    | Não        | `build/`, `.class`, binários e temporários são ignorados |

---

## Estrutura do Repositório

```text
ppgia-puc-pr-asc/
|-- atividades/
|   |-- hypervisor-vm-host/    # virtualização, coleta de métricas e relatório
|   |-- produto-consumidor/    # Java, fila bloqueante e concorrência
|   |-- contador-caracteres/   # Java, leitura multithread e análise de threads
|   |-- rtos-determinismo/     # C, Linux RT, Zephyr/QEMU e gráficos
|   `-- protocolo-rpl/         # revisão bibliográfica sobre roteamento RPL
|-- docs/
|   `-- edf-trade-offs.md
|-- literatura/
|   `-- referencias.bib        # bibliografia centralizada, quando aplicável
|-- CITATION.cff               # metadados de citação, quando versionado
|-- mise.toml                  # automação local opcional
`-- README.md
```

---

## Como Referenciar

Para citar este repositório em trabalhos acadêmicos ou documentação técnica:

```bibtex
@software{Dantas2026ASC,
  author      = {Dantas, F. S.},
  title       = {Arquitetura de Sistemas Computacionais: Repositório de Experimentos},
  year        = {2026},
  institution = {PPGIA/PUC-PR},
  url         = {https://github.com/fsd-dantas/ppgia-puc-pr-asc}
}
```

Quando versionado, o arquivo `CITATION.cff` contém metadados estruturados para ferramentas como GitHub e Zenodo.
