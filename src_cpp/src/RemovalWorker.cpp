#include "RemovalWorker.h"

#include "ModelManager.h"

#include <utility>

namespace rembg::native_app {

RemovalWorker::RemovalWorker(QObject* parent) : QObject(parent) {}

void RemovalWorker::process(const QString& fileName, const QImage& image) {
    auto modelPath = ModelManager::ensureU2NetModel(
        [this, fileName](const QString& message) { emit statusChanged(fileName, message); },
        [this, fileName](qint64 bytesReceived, qint64 bytesTotal) {
            emit progressChanged(fileName, bytesReceived, bytesTotal);
        }
    );

    if (!modelPath) {
        emit failed(fileName, modelPath.error());
        return;
    }

    if (!engine_ || loadedModelPath_ != *modelPath) {
        emit statusChanged(fileName, "Loading ONNX model...");
        auto engine = BackgroundRemovalEngine::create(*modelPath);
        if (!engine) {
            emit failed(fileName, engine.error());
            return;
        }

        loadedModelPath_ = *modelPath;
        engine_ = std::move(*engine);
    }

    emit statusChanged(fileName, "Removing background...");
    auto result = engine_->removeBackground(image);
    if (!result) {
        emit failed(fileName, result.error());
        return;
    }

    emit finished(fileName, result->image, result->mask);
}

}  // namespace rembg::native_app
