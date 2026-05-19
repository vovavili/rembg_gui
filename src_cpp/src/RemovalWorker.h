#pragma once

#include "BackgroundRemovalEngine.h"

#include <filesystem>
#include <memory>

#include <QImage>
#include <QObject>

namespace rembg::native_app {

class RemovalWorker : public QObject {
    Q_OBJECT

public:
    explicit RemovalWorker(QObject* parent = nullptr);

public slots:
    void process(const QString& fileName, const QImage& image);

signals:
    void statusChanged(const QString& fileName, const QString& message);
    void progressChanged(const QString& fileName, qint64 bytesReceived, qint64 bytesTotal);
    void finished(const QString& fileName, const QImage& output, const QImage& mask);
    void failed(const QString& fileName, const QString& message);

private:
    std::unique_ptr<BackgroundRemovalEngine> engine_;
    std::filesystem::path loadedModelPath_;
};

}  // namespace rembg::native_app
