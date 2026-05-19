#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/../.." && pwd)"

build_type="${CMAKE_BUILD_TYPE:-Release}"
build_dir="${BUILD_DIR:-${repo_root}/build}"
vcpkg_root="${VCPKG_ROOT:-${repo_root}/vcpkg}"
artifact_name="${ARTIFACT_NAME:-rembg-gui-linux-x64.AppImage}"

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

rm -rf "${repo_root}/AppDir" "${repo_root:?}/${artifact_name}"
cmake --install "${build_dir}" --config "${build_type}" --prefix "${repo_root}/AppDir/usr"

linuxdeploy="${repo_root}/linuxdeploy-x86_64.AppImage"
linuxdeploy_qt="${repo_root}/linuxdeploy-plugin-qt-x86_64.AppImage"

curl -L -o "${linuxdeploy}" \
  https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
curl -L -o "${linuxdeploy_qt}" \
  https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x "${linuxdeploy}" "${linuxdeploy_qt}"

export APPIMAGE_EXTRACT_AND_RUN="${APPIMAGE_EXTRACT_AND_RUN:-1}"
export QMAKE="${QMAKE:-${VCPKG_INSTALLED_DIR}/${VCPKG_DEFAULT_TRIPLET}/tools/Qt6/bin/qmake}"
export LD_LIBRARY_PATH="${repo_root}/AppDir/usr/lib:${VCPKG_INSTALLED_DIR}/${VCPKG_DEFAULT_TRIPLET}/lib:${LD_LIBRARY_PATH:-}"

(
  cd "${repo_root}"
  "${linuxdeploy}" \
    --appdir AppDir \
    --executable AppDir/usr/bin/rembg-gui \
    --desktop-file src_cpp/packaging/rembg-gui.desktop \
    --icon-file src_cpp/packaging/rembg-gui.svg \
    --plugin qt \
    --output appimage

  generated_appimage="$(find . -maxdepth 1 -name '*.AppImage' ! -name 'linuxdeploy*.AppImage' -print -quit)"
  test -n "${generated_appimage}"
  mv "${generated_appimage}" "${artifact_name}"
)
