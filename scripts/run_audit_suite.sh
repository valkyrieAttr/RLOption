#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
compiler="${CXX:-clang++}"
compiler_name="${compiler##*/}"
build_dir="${RLOPTION_BUILD_DIR:-${project_root}/build/audit-${compiler_name}}"
jobs="${RLOPTION_JOBS:-1}"
test_timeout="${RLOPTION_TEST_TIMEOUT:-7200}"

cmake -S "${project_root}" -B "${build_dir}" \
    -DCMAKE_CXX_COMPILER="${compiler}" \
    -DRLOPTION_BUILD_TEST_SUITE=ON

cmake --build "${build_dir}" --target a.out test-suites --parallel "${jobs}"
ctest --test-dir "${build_dir}" --output-on-failure \
    --timeout "${test_timeout}"

ulimit -c 0
expected_failure="${build_dir}/test_storage_reentrancy_rejected_mutation"
log_file="$(mktemp)"
trap 'rm -f "${log_file}"' EXIT

set +e
"${expected_failure}" >"${log_file}" 2>&1
status="$?"
set -e

if [[ "${status}" -ne 134 ]] ||
   ! grep -Fq "cannot mutate Option while an alternative is being destroyed" "${log_file}"; then
    cat "${log_file}" >&2
    printf 'expected storage reentrancy test to abort with SIGABRT and its guard message; got status %s\n' "${status}" >&2
    exit 1
fi

printf 'storage_reentrancy_rejected_mutation: expected SIGABRT guard observed\n'
