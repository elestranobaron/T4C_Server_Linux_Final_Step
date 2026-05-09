#!/bin/bash
# On évite les chemins absolus Windows (mnt/c/...) si tu es déjà dans le dossier sur Arch
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

# Nettoyage propre
rm -rf build
mkdir build
cd build

# On force le compilateur et le standard C++
# -j$(nproc) permet d'utiliser tous tes cœurs CPU pour compiler plus vite
cmake .. -DCMAKE_CXX_STANDARD=17
if [ $? -eq 0 ]; then
    make -j$(nproc)
else
    echo "Erreur lors de la configuration CMake"
    exit 1
fi