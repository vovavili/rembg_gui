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

That script is also what GitHub Actions runs for the Linux job. It configures,
builds, runs `ctest` with `REMBG_NATIVE_RUN_MODEL_TESTS=1`, installs into
`AppDir`, and builds the AppImage with linuxdeploy.

## Release layout

Releases keep `gui.py` at the repository root and publish a Windows x64 zip
bundle plus a Linux x64 AppImage. The ONNX model is not bundled with the
binaries.

The Windows zip opens with `rembg-gui.exe` in the top folder. That file is a
small launcher; the real Qt app and its DLLs stay in `bin/` as
`rembg-gui-app.exe`. Users should extract the zip and run the launcher from the
extracted folder.
