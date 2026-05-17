#!/bin/bash
# Compile T4CServer, puis synchronise res/ → build/ (données runtime).
# res/ est la source de vérité : T4CServer.ini, WDA/, textfilter.ini, etc.
# rm -rf build ne détruit donc jamais la config ni les données.
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
RES_DIR="$PROJECT_DIR/res"
BUILD_DIR="$PROJECT_DIR/build"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"
cmake .. -DCMAKE_CXX_STANDARD=17
make -j"$(nproc)"

# Synchronise res/ → build/ (copie récursive, écrase si plus récent)
if [[ -d "$RES_DIR" ]]; then
    rsync -a --exclude='readme' --exclude='*.md' "$RES_DIR/" "$BUILD_DIR/"
    echo "[build.sh] res/ → build/ synchronisé."
fi

echo "[build.sh] OK : $BUILD_DIR/T4CServer"
