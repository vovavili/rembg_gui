#include "ModelManager.h"

#include "PathUtils.h"

#include <utility>

#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

namespace rembg::native_app {

namespace {

const QUrl kU2NetUrl(
    QStringLiteral("https://github.com/danielgatis/rembg/releases/download/v0.0.0/u2net.onnx")
);
constexpr auto kU2NetFileName = "u2net.onnx";
constexpr auto kU2NetMd5 = "60024c5c889badc19c04ad937298a77b";

}  // namespace

Expected<std::filesystem::path> ModelManager::ensureU2NetModel(
    StatusCallback statusCallback,
    ProgressCallback progressCallback
) {
    const std::filesystem::path directory = modelDirectory();
    const QString directoryPath = pathToQString(directory);

    if (!QDir().mkpath(directoryPath)) {
        return failure("Cannot create model cache directory: " + directoryPath);
    }

    const std::filesystem::path modelPath = directory / kU2NetFileName;

    if (std::filesystem::exists(modelPath)) {
        if (hasExpectedMd5(modelPath, QString::fromLatin1(kU2NetMd5))) {
            if (statusCallback) {
                statusCallback("Using cached U2Net model.");
            }
            return modelPath;
        }

        if (statusCallback) {
            statusCallback("Cached model checksum failed. Downloading a fresh copy...");
        }
        QFile::remove(pathToQString(modelPath));
    }

    return downloadModel(kU2NetUrl, modelPath, std::move(statusCallback), std::move(progressCallback));
}

std::filesystem::path ModelManager::modelDirectory() {
    const QString configuredHome = qEnvironmentVariable("U2NET_HOME");
    if (!configuredHome.isEmpty()) {
        return qStringToPath(configuredHome);
    }

    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appData.isEmpty()) {
        appData = QDir::homePath() + "/.rembg_gui";
    }

    return qStringToPath(appData + "/models");
}

bool ModelManager::hasExpectedMd5(const std::filesystem::path& filePath, const QString& expectedMd5) {
    QFile file(pathToQString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    while (!file.atEnd()) {
        hash.addData(file.read(1024 * 1024));
    }

    return QString::fromLatin1(hash.result().toHex()).compare(expectedMd5, Qt::CaseInsensitive) == 0;
}

Expected<std::filesystem::path> ModelManager::downloadModel(
    const QUrl& url,
    const std::filesystem::path& destination,
    StatusCallback statusCallback,
    ProgressCallback progressCallback
) {
    if (statusCallback) {
        statusCallback("Downloading U2Net model...");
    }

    const QString destinationPath = pathToQString(destination);
    const QString temporaryPath = destinationPath + ".download";
    QFile::remove(temporaryPath);

    QFile outputFile(temporaryPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        return failure("Cannot write model file: " + temporaryPath);
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::NoLessSafeRedirectPolicy
    );

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;

    QObject::connect(reply, &QNetworkReply::readyRead, [&]() {
        outputFile.write(reply->readAll());
    });
    QObject::connect(reply, &QNetworkReply::downloadProgress, [&](qint64 received, qint64 total) {
        if (progressCallback) {
            progressCallback(received, total);
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    loop.exec();
    outputFile.write(reply->readAll());
    outputFile.close();

    const QNetworkReply::NetworkError error = reply->error();
    const QString errorString = reply->errorString();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        QFile::remove(temporaryPath);
        return failure("Model download failed: " + errorString);
    }

    if (!hasExpectedMd5(qStringToPath(temporaryPath), QString::fromLatin1(kU2NetMd5))) {
        QFile::remove(temporaryPath);
        return failure("Downloaded model did not match the expected checksum.");
    }

    QFile::remove(destinationPath);
    if (!QFile::rename(temporaryPath, destinationPath)) {
        QFile::remove(temporaryPath);
        return failure("Cannot move downloaded model into the cache.");
    }

    if (statusCallback) {
        statusCallback("U2Net model is ready.");
    }

    return destination;
}

}  // namespace rembg::native_app
