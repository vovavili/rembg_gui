#pragma once

#include "Expected.h"

#include <filesystem>
#include <functional>

#include <QUrl>
#include <QtGlobal>

namespace rembg::native_app {

class ModelManager {
public:
    using StatusCallback = std::function<void(const QString&)>;
    using ProgressCallback = std::function<void(qint64, qint64)>;

    static Expected<std::filesystem::path> ensureU2NetModel(
        StatusCallback statusCallback = {},
        ProgressCallback progressCallback = {}
    );

    static std::filesystem::path modelDirectory();
    static bool hasExpectedMd5(const std::filesystem::path& filePath, const QString& expectedMd5);

private:
    static Expected<std::filesystem::path> downloadModel(
        const QUrl& url,
        const std::filesystem::path& destination,
        StatusCallback statusCallback,
        ProgressCallback progressCallback
    );
};

}  // namespace rembg::native_app
