#include "BackgroundRemovalEngine.h"

#include "PathUtils.h"

#include <algorithm>
#include <array>
#include <exception>
#include <span>
#include <utility>
#include <vector>

#include <QThread>

namespace rembg::native_app {

namespace {

constexpr QSize kU2NetInputSize{320, 320};

}  // namespace

BackgroundRemovalEngine::BackgroundRemovalEngine()
    : env_(ORT_LOGGING_LEVEL_WARNING, "rembg_gui") {}

Expected<std::unique_ptr<BackgroundRemovalEngine>> BackgroundRemovalEngine::create(
    const std::filesystem::path& modelPath
) {
    if (!std::filesystem::exists(modelPath)) {
        return failure("Model file does not exist: " + pathToQString(modelPath));
    }

    std::unique_ptr<BackgroundRemovalEngine> engine;

    try {
        engine = std::unique_ptr<BackgroundRemovalEngine>(new BackgroundRemovalEngine());

        const int threadCount = std::clamp(QThread::idealThreadCount(), 1, 4);
        engine->sessionOptions_.SetIntraOpNumThreads(threadCount);
        engine->sessionOptions_.SetInterOpNumThreads(1);
        engine->sessionOptions_.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        engine->sessionOptions_.DisableMemPattern();
        engine->sessionOptions_.DisableCpuMemArena();
        engine->sessionOptions_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_DISABLE_ALL);

#ifdef _WIN32
        const std::wstring nativePath = modelPath.wstring();
#else
        const std::string nativePath = modelPath.string();
#endif

        engine->session_ = std::make_unique<Ort::Session>(
            engine->env_,
            nativePath.c_str(),
            engine->sessionOptions_
        );

        Ort::AllocatorWithDefaultOptions allocator;
        Ort::AllocatedStringPtr inputName =
            engine->session_->GetInputNameAllocated(0, allocator);
        Ort::AllocatedStringPtr outputName =
            engine->session_->GetOutputNameAllocated(0, allocator);

        engine->inputName_ = inputName.get();
        engine->outputName_ = outputName.get();
    } catch (const Ort::Exception& error) {
        return failure(QStringLiteral("ONNX Runtime failed to load the model: %1")
                           .arg(QString::fromUtf8(error.what())));
    } catch (const std::bad_alloc&) {
        return failure(QStringLiteral("ONNX Runtime session setup ran out of memory."));
    } catch (const std::exception& error) {
        return failure(QStringLiteral("ONNX Runtime session setup failed: %1")
                           .arg(QString::fromUtf8(error.what())));
    }

    return engine;
}

Expected<RemovalResult> BackgroundRemovalEngine::removeBackground(const QImage& image) {
    if (!session_) {
        return failure("The ONNX session is not initialized.");
    }

    auto tensor = makeU2NetInput(image, kU2NetInputSize);
    if (!tensor) {
        return failure(tensor.error());
    }

    std::array<int64_t, 4> inputShape{
        1,
        3,
        static_cast<int64_t>(tensor->size.height()),
        static_cast<int64_t>(tensor->size.width()),
    };

    try {
        Ort::MemoryInfo memoryInfo =
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            tensor->values.data(),
            tensor->values.size(),
            inputShape.data(),
            inputShape.size()
        );

        const char* inputNames[] = {inputName_.c_str()};
        const char* outputNames[] = {outputName_.c_str()};
        std::vector<Ort::Value> outputTensors = session_->Run(
            Ort::RunOptions{nullptr},
            inputNames,
            &inputTensor,
            1,
            outputNames,
            1
        );

        if (outputTensors.empty() || !outputTensors.front().IsTensor()) {
            return failure("The model did not return an output tensor.");
        }

        Ort::Value& outputTensor = outputTensors.front();
        auto tensorInfo = outputTensor.GetTensorTypeAndShapeInfo();
        const size_t elementCount = tensorInfo.GetElementCount();
        const size_t maskElementCount =
            static_cast<size_t>(kU2NetInputSize.width() * kU2NetInputSize.height());

        if (elementCount < maskElementCount) {
            return failure("The model output is smaller than the expected U2Net mask.");
        }

        const float* outputData = outputTensor.GetTensorData<float>();
        auto mask = maskFromPrediction(
            std::span<const float>(outputData, maskElementCount),
            kU2NetInputSize
        );
        if (!mask) {
            return failure(mask.error());
        }

        QImage fullSizeMask = mask->scaled(
                                       image.size(),
                                       Qt::IgnoreAspectRatio,
                                       Qt::SmoothTransformation
        )
                                  .convertToFormat(QImage::Format_Grayscale8);

        auto output = applyMaskAsAlpha(image, fullSizeMask);
        if (!output) {
            return failure(output.error());
        }

        return RemovalResult{.image = *output, .mask = std::move(fullSizeMask)};
    } catch (const Ort::Exception& error) {
        return failure(QStringLiteral("ONNX Runtime inference failed: %1")
                           .arg(QString::fromUtf8(error.what())));
    } catch (const std::bad_alloc&) {
        return failure(QStringLiteral("ONNX Runtime inference ran out of memory."));
    } catch (const std::exception& error) {
        return failure(QStringLiteral("ONNX Runtime inference failed: %1")
                           .arg(QString::fromUtf8(error.what())));
    }
}

}  // namespace rembg::native_app
