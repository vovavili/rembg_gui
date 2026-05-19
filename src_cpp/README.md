# C++ build

This is the C++23/QtWidgets port of `gui.py`. The UI stays close to the
original PyQt6 script, while the background-removal path calls ONNX Runtime
directly instead of importing `rembg`.

## Dependencies

Use CMake with vcpkg manifest mode. vcpkg provides Qt. During configure, CMake
downloads the official ONNX Runtime binary, or you can pass `ONNXRUNTIME_ROOT`
to an already extracted release.

```powershell
cmake -S src_cpp -B src_cpp/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build src_cpp/build --config Release
ctest --test-dir src_cpp/build --output-on-failure
cmake --install src_cpp/build --config Release --prefix "$PWD\dist\rembg-gui"
```

## Model cache

The app downloads `u2net.onnx` on first use, verifies its MD5 checksum, and
stores it in the Qt app data directory. Set `U2NET_HOME` to use a custom model
directory.

## Local Linux CI

The release workflow is reproducible in the devcontainer. Open the repository
in VS Code, choose "Reopen in Container", then run:

```bash
bash src_cpp/scripts/ci-linux.sh
```

That script uses Ubuntu 22.04 packages, vcpkg manifest mode, a Release build,
and `REMBG_NATIVE_RUN_MODEL_TESTS=1`, matching the Linux release job closely
enough to catch the model download path before pushing.

## Release layout

Releases keep `gui.py` at the repository root and publish a Windows x64 zip
bundle plus a Linux x64 AppImage. The ONNX model is not bundled with the
binaries.
