#!/usr/bin/env bash

set -euo pipefail

MEDIA_BACKEND="${1:-nvmpi}"

cmake -S . -B "build-aarch64-${MEDIA_BACKEND}" -DMEDIA_BACKEND="${MEDIA_BACKEND}"
cmake --build "build-aarch64-${MEDIA_BACKEND}" -j"$(nproc)"
