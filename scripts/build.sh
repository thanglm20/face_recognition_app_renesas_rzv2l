#!/usr/bin/env bash
set -euo pipefail

APP_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TVM_ROOT="${TVM_ROOT:-}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-${TVM_ROOT}/apps/toolchain/runtime.cmake}"
BUILD_DIR="${BUILD_DIR:-${APP_ROOT}/build}"
JOBS="${JOBS:-$(nproc)}"
TARGET="face_recognition_drpai_tvm_ws"

if [[ -z "${TVM_ROOT}" ]]; then
    echo "ERROR: Set TVM_ROOT to the DRP-AI TVM directory." >&2
    echo "Example: export TVM_ROOT=/path/to/drp-ai_tvm/tvm" >&2
    exit 1
fi

if [[ ! -f "${TOOLCHAIN_FILE}" ]]; then
    echo "ERROR: CMake toolchain file not found: ${TOOLCHAIN_FILE}" >&2
    echo "Set TOOLCHAIN_FILE if your toolchain is installed elsewhere." >&2
    exit 1
fi

mkdir -p "${BUILD_DIR}"

cmake -S "${APP_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" ..

    
echo "Starting build for target: ${TARGET}"
cmake --build "${BUILD_DIR}" --target "${TARGET}" --parallel "${JOBS}"

BIN="${BUILD_DIR}/${TARGET}"
if [[ ! -f "${BIN}" ]]; then
    echo "ERROR: Build completed but executable was not found: ${BIN}" >&2
    exit 1
fi

mkdir -p "${APP_ROOT}/exe"
cp "${BIN}" "${APP_ROOT}/exe/${TARGET}"
echo "Copied executable to ${APP_ROOT}/exe/${TARGET}"