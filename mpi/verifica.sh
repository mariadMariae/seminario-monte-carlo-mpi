#!/bin/sh
# Verifica a versão MPI: compila e roda com 1, 2, 4 e 8 processos.
# Confere, para cada P:
#   1. o resultado fica a menos de 3 sigma de PI/4;
#   2. o sigma é o mesmo do P = 1 (tolerância de 5 %), pois o total de pontos é igual.
# Uso: sh mpi/verifica.sh [calls]   (padrão: 10000000)
# Sai com código 1 se alguma verificação falhar.

set -e

CALLS=${1:-10000000}
DIR=$(cd "$(dirname "$0")" && pwd)
BIN=$(mktemp -d)/monte_carlo_mpi
trap 'rm -rf "$(dirname "$BIN")"' EXIT

mpicc -O2 -Wall -Wextra "$DIR/monte_carlo_mpi.c" -o "$BIN" -lgsl -lgslcblas -lm

# O Open MPI recusa mais processos que núcleos sem --oversubscribe.
EXTRA=""
if command -v ompi_info > /dev/null 2>&1; then
  EXTRA="--oversubscribe"
fi

FALHAS=0
SIGMA1=""

for P in 1 2 4 8; do
  if ! SAIDA=$(mpirun $EXTRA -np "$P" "$BIN" "$CALLS"); then
    echo "P = $P  FALHA: mpirun terminou com erro"
    FALHAS=$((FALHAS + 1))
    continue
  fi
  RES=$(echo "$SAIDA" | awk '/^result/ {print $3}')
  SIGMA=$(echo "$SAIDA" | awk '/^sigma/ {print $3}')
  TEMPO=$(echo "$SAIDA" | awk '/^tempo/ {print $3}')
  [ -z "$SIGMA1" ] && SIGMA1=$SIGMA

  STATUS=$(awk -v r="$RES" -v s="$SIGMA" -v s1="$SIGMA1" 'BEGIN {
    exato = atan2(1, 1)
    d = r - exato; if (d < 0) d = -d
    ds = s - s1; if (ds < 0) ds = -ds
    if (s <= 0 || d >= 3 * s)  { print "FALHA: resultado a mais de 3 sigma"; exit }
    if (ds > 0.05 * s1)        { print "FALHA: sigma diferente do P = 1"; exit }
    printf "OK (%.2f sigma)", d / s
  }')

  echo "P = $P  result = $RES  sigma = $SIGMA  tempo = $TEMPO s  $STATUS"
  case "$STATUS" in FALHA*) FALHAS=$((FALHAS + 1)) ;; esac
done

if [ "$FALHAS" -gt 0 ]; then
  echo "$FALHAS verificação(ões) falharam"
  exit 1
fi
echo "Todas as verificações passaram"
