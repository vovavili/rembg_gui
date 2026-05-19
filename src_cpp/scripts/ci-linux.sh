#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/../.." && pwd)"

build_type="${CMAKE_BUILD_TYPE:-Release}"
build_dir="${BUILD_DIR:-${repo_root}/build-linux-ci}"
vcpkg_root="${VCPKG_ROOT:-${repo_root}/vcpkg}"

if [[ ! -x "${vcpkg_root}/vcpkg" ]]; then
  if [[ ! -d "${vcpkg_root}" ]]; then
    git clone https://github.com/microsoft/vcpkg.git "${vcpkg_root}"
  fi
  "${vcpkg_root}/bootstrap-vcpkg.sh"
fi

export VCPKG_DEFAULT_TRIPLET="${VCPKG_DEFAULT_TRIPLET:-x64-linux}"
export VCPKG_INSTALLED_DIR="${VCPKG_INSTALLED_DIR:-${repo_root}/vcpkg_installed}"
export REMBG_NATIVE_RUN_MODEL_TESTS="${REMBG_NATIVE_RUN_MODEL_TESTS:-1}"
export U2NET_HOME="${U2NET_HOME:-${build_dir}/model-cache}"

cmake -S "${repo_root}/src_cpp" -B "${build_dir}" \
  -DCMAKE_BUILD_TYPE="${build_type}" \
  -DCMAKE_TOOLCHAIN_FILE="${vcpkg_root}/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET="${VCPKG_DEFAULT_TRIPLET}" \
  -DVCPKG_INSTALLED_DIR="${VCPKG_INSTALLED_DIR}"

cmake --build "${build_dir}" --config "${build_type}" --parallel
ctest --test-dir "${build_dir}" --output-on-failure -C "${build_type}"
