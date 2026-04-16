#!/usr/bin/env bash
# coleta.sh — script auxiliar para coleta de dados dos experimentos
#
# Repositório: https://github.com/fsd-dantas/ppgia-puc-pr-asc
# Atividade:   hypervisor-vm-host
# Disciplina:  Arquitetura de Sistemas Computacionais — PPGIA/PUC-PR
#
# Executar no GUEST durante cada experimento.
# Uso: ./coleta.sh <nome-experimento>
# Exemplo: ./coleta.sh exp1-cpu

set -euo pipefail

EXP="${1:-exp}"
OUT="../dados/${EXP}"
mkdir -p "$OUT"

echo "[$(date)] Iniciando coleta: $EXP" | tee "$OUT/coleta.log"

# Snapshot inicial do sistema
uname -a        > "$OUT/uname.txt"
lscpu           > "$OUT/lscpu.txt"
free -h         > "$OUT/free.txt"
cat /proc/meminfo > "$OUT/meminfo.txt"
cat /proc/cpuinfo > "$OUT/cpuinfo.txt"

echo "Arquivos de baseline salvos em $OUT/"
echo "Inicie o experimento e monitore com:"
echo "  iostat -xz 1 | tee $OUT/iostat.txt"
echo "  vmstat 1     | tee $OUT/vmstat.txt"
echo "  top -b -n 30 -d 2 | tee $OUT/top.txt"
