#pragma once

#include "Expected.h"
#include "ImageProcessing.h"

#include <filesystem>
#include <memory>
#include <string>

#include <onnxruntime_cxx_api.h>

namespace rembg::native_app {

class BackgroundRemovalEngine {
public:
    static Expected<std::unique_ptr<BackgroundRemovalEngine>> create(
        const std::filesystem::path& modelPath
    );

    Expected<RemovalResult> removeBackground(const QImage& image);

private:
    BackgroundRemovalEngine();

    Ort::Env env_;
    Ort::SessionOptions sessionOptions_;
    std::unique_ptr<Ort::Session> session_;
    std::string inputName_;
    std::string outputName_;
};

}  // namespace rembg::native_app
