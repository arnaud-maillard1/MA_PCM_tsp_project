#!/usr/bin/env bash

# EXE=../tsp
# INPUT="../dj38.tsp"
# SIZES=(17 18)
# RESULTS="../results.csv"

# # THREADS=(2 4 8 16 32 64 128 256)
# # CUTOFFS=(3 4 5 6 7 8)

# THREADS=(256)
# CUTOFFS=(2 4 5 6 7 8)

# RUNS=10

# echo "file;size;threads;cutoff;T_seq;T_par;speedup;efficiency" > "$RESULTS"

# for size in "${SIZES[@]}"; do
#     for c in "${CUTOFFS[@]}"; do
#         for t in "${THREADS[@]}"; do
#             echo "Running: size=$size threads=$t cutoff=$c (x$RUNS)"

#             for run in $(seq 1 $RUNS); do
#                 echo -n "  => run $run ... "
#                 $EXE "$INPUT" $size $t $c >> "$RESULTS"
#                 echo "done"
#             done
#             echo
#         done
#     done
# done

# set -euo pipefail

# BIN=./tsp
# INSTANCE=dj38.tsp
# GRAPH_SIZE=16
# THREADS=256
# RESULTS=results_budget.csv
# RUNS=3

# echo "file;size;threads;budget;run;time" > "$RESULTS"

# for BUDGET in $(seq 25000 500 35000); do
#  echo "Budget = ${BUDGET}"

#  for RUN in $(seq 1 $RUNS); do
#    LINE=$(${BIN} "${INSTANCE}" "${GRAPH_SIZE}" "${THREADS}" "${BUDGET}" | head -n 1)

#    echo "${LINE}" >> "$RESULTS"
#  done
# done

# echo "Fin des mesures !"


# set -euo pipefail

# BIN=./tsp
# INSTANCE=dj38.tsp
# GRAPH_SIZE=16
# THREADS=1
# BUDGET=1
# RESULTS=results_budget.csv
# RUNS=5

# echo "file;size;threads;budget;run;time" > "$RESULTS"

# echo "Exécution avec Budget fixe = ${BUDGET}"

# for RUN in $(seq 1 $RUNS); do
#     # Exécution du programme avec le budget de 35000
#     LINE=$(${BIN} "${INSTANCE}" "${GRAPH_SIZE}" "${THREADS}" "${BUDGET}" | head -n 1)
    
#     echo "Run ${RUN} terminé"
#     echo "${LINE}" >> "$RESULTS"
# done

# echo "Fin des mesures ! Les résultats sont dans ${RESULTS}"

set -euo pipefail

BIN=../src/tsp
INSTANCE=../cities-files-examples/dj38.tsp
GRAPH_SIZE=14
RUNS=3
RESULTS=results_budget.csv

THREADS_LIST=(2 4 8 16 32 64 128 192 256)

echo "file;size;threads;budget;run;time;open;closed;path" > "$RESULTS"

for THREADS in "${THREADS_LIST[@]}"; do
  BUDGET=$((111 * THREADS))
  echo "=== Threads=${THREADS} | Budget=${BUDGET} | Size=${GRAPH_SIZE} ==="

  for RUN in $(seq 1 "$RUNS"); do
    LINE="$(${BIN} "${INSTANCE}" "${GRAPH_SIZE}" "${THREADS}" "${BUDGET}" | head -n 1)"
    echo "  Run ${RUN} terminé"
    echo "${LINE}" >> "$RESULTS"
  done
done

echo "Fin des mesures ! Les résultats sont dans ${RESULTS}"
