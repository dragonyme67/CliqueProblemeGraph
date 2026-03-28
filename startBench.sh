#!/bin/bash

# Vérification des arguments
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <nb_iterations> <instance1.clq> [instance2.clq ...]"
    exit 1
fi

NB_ITER=$1
shift 

# 1. Compilation
echo "--- Compiling C++ ---"
make -j$(nproc) # Utilise tous les cœurs du Xeon pour compiler plus vite

# 2. Exécution C++
echo "--- Running Benchmark ($NB_ITER iterations) ---"
./cliqueSolver "$NB_ITER" "$@"

# 3. Transfert des données
PYTHON_DIR="../pythonGraph"
CSV_FILE="benchmark_results.csv"

if [ -f "$CSV_FILE" ]; then
    echo "--- Moving results to $PYTHON_DIR ---"
    mv "$CSV_FILE" "$PYTHON_DIR/"
else
    echo "Error: $CSV_FILE not found!"
    exit 1
fi

# 4. Analyse Python via le VENV
echo "--- Launching Scientific Analysis ---"
cd "$PYTHON_DIR" || exit

if [ -x "./venv/bin/python3" ]; then
    ./venv/bin/python3 main.py
else
    echo "Error: Virtual environment not found in $(pwd)/venv"
    echo "Please create it with: python3 -m venv venv"
    exit 1
fi

echo "--- All done! ---"
