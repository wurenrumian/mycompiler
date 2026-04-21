#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
COMPILER_PATH=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --compiler)
      COMPILER_PATH="${2:-}"
      shift 2
      ;;
    --project-root)
      PROJECT_ROOT="${2:-}"
      shift 2
      ;;
    *)
      echo "[ERROR] unknown argument: $1"
      echo "Usage: $0 [--compiler <path>] [--project-root <path>]"
      exit 2
      ;;
  esac
done

if [[ -z "${COMPILER_PATH}" ]]; then
  if [[ -x "${PROJECT_ROOT}/build/compiler" ]]; then
    COMPILER_PATH="${PROJECT_ROOT}/build/compiler"
  elif [[ -x "${PROJECT_ROOT}/build/compiler.exe" ]]; then
    COMPILER_PATH="${PROJECT_ROOT}/build/compiler.exe"
  else
    echo "[ERROR] compiler binary not found. pass --compiler <path>."
    exit 2
  fi
fi

if [[ ! -x "${COMPILER_PATH}" ]]; then
  echo "[ERROR] compiler is not executable: ${COMPILER_PATH}"
  exit 2
fi

PUBLIC_DIRS=(
  "${PROJECT_ROOT}/public/functional_easy"
  "${PROJECT_ROOT}/public/functional_hard"
  "${PROJECT_ROOT}/public/performance_easy"
)

cases=()
for d in "${PUBLIC_DIRS[@]}"; do
  if [[ -d "${d}" ]]; then
    while IFS= read -r -d '' f; do
      cases+=("$f")
    done < <(find "${d}" -type f -name "*.sy" -print0)
  fi
done

if [[ ${#cases[@]} -eq 0 ]]; then
  echo "[WARN] no .sy files found under public/*"
  exit 0
fi

OUTPUT_ROOT="${PROJECT_ROOT}/build/public_test_out"
mkdir -p "${OUTPUT_ROOT}"

passed=0
failed=0

for case_file in "${cases[@]}"; do
  rel="${case_file#"${PROJECT_ROOT}/"}"
  out_file="${OUTPUT_ROOT}/$(basename "${case_file}" .sy).s"

  echo "[RUN] ${rel}"
  if ! "${COMPILER_PATH}" -S -o "${out_file}" "${case_file}"; then
    echo "[FAIL] compiler returned non-zero"
    failed=$((failed + 1))
    continue
  fi

  if [[ ! -f "${out_file}" ]]; then
    echo "[FAIL] output .s file missing"
    failed=$((failed + 1))
    continue
  fi

  if [[ ! -s "${out_file}" ]]; then
    echo "[FAIL] output .s file is empty"
    failed=$((failed + 1))
    continue
  fi

  echo "[PASS]"
  passed=$((passed + 1))
done

echo
echo "[SUMMARY] passed = ${passed}, failed = ${failed}"

if [[ ${failed} -ne 0 ]]; then
  exit 1
fi

exit 0
