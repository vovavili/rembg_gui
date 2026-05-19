# C++ build

This is the C++23/QtWidgets port of `gui.py`. The UI stays close to the
original PyQt6 script, while the background-removal path calls ONNX Runtime
directly instead of importing `rembg`.

## Dependencies

Use CMake with vcpkg manifest mode. vcpkg provides Qt. During configure, CMake
downloads the official ONNX Runtime binary, or you can pass `ONNXRUNTIME_ROOT`
to an already extracted release.

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\path\to\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
ctest --test-dir build --output-on-failure
cmake --install build --config Release --prefix "$PWD\dist\rembg-gui"
```

## Model cache

The app downloads `u2net.onnx` on first use, verifies its MD5 checksum, and
stores it in the Qt app data directory. Set `U2NET_HOME` to use a custom model
directory.

## Release layout

Releases keep `gui.py` at the repository root and publish a Windows x64 zip
bundle plus a Linux x64 AppImage. The ONNX model is not bundled with the
binaries.
