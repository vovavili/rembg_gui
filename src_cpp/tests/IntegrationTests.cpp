#include "BackgroundRemovalEngine.h"
#include "ModelManager.h"

#include <algorithm>
#include <exception>
#include <cstdio>

#include <QImage>
#include <QNetworkAccessManager>
#include <QSslSocket>
#include <QStringList>
#include <QtTest>

namespace rembg::native_app {

class IntegrationTests : public QObject {
    Q_OBJECT

private slots:
    void removesBackgroundWhenModelTestsAreEnabled() {
        if (!qEnvironmentVariableIsSet("REMBG_NATIVE_RUN_MODEL_TESTS")) {
            QSKIP("Set REMBG_NATIVE_RUN_MODEL_TESTS=1 to run the ONNX integration test.");
        }

        try {
            runEnabledModelTest();
        } catch (const std::exception& error) {
            const QByteArray message =
                QStringLiteral("Unhandled C++ exception: %1").arg(QString::fromUtf8(error.what()))
                    .toUtf8();
            QFAIL(message.constData());
        } catch (...) {
            QFAIL("Unhandled non-standard exception.");
        }
    }

private:
    static void failWithMessage(const QString& message) {
        const QByteArray bytes = message.toUtf8();
        std::fprintf(stderr, "%s\n", bytes.constData());
        QFAIL(bytes.constData());
    }

    void requireHttpsSupport() {
        QNetworkAccessManager manager;
        const QStringList schemes = manager.supportedSchemes();
        if (!schemes.contains(QStringLiteral("https"), Qt::CaseInsensitive)) {
            failWithMessage(
                QStringLiteral("Qt Network does not support HTTPS. Supported schemes: %1")
                    .arg(schemes.join(", "))
            );
        }

        if (!QSslSocket::supportsSsl()) {
            failWithMessage(
                QStringLiteral("Qt SSL is unavailable. Built against: %1. Runtime: %2.")
                    .arg(
                        QSslSocket::sslLibraryBuildVersionString(),
                        QSslSocket::sslLibraryVersionString()
                    )
            );
        }
    }

    void runEnabledModelTest() {
        QImage image(QString::fromUtf8(REMBG_NATIVE_SAMPLE_IMAGE));
        QVERIFY2(!image.isNull(), "The sample image could not be loaded.");

        requireHttpsSupport();

        auto modelPath = ModelManager::ensureU2NetModel();
        if (!modelPath.has_value()) {
            failWithMessage(modelPath.error());
        }

        auto engine = BackgroundRemovalEngine::create(*modelPath);
        if (!engine.has_value()) {
            failWithMessage(engine.error());
        }

        auto result = (*engine)->removeBackground(image);
        if (!result.has_value()) {
            failWithMessage(result.error());
        }
        QCOMPARE(result->image.size(), image.size());
        QVERIFY(result->image.hasAlphaChannel());
        QCOMPARE(result->mask.size(), image.size());

        int minAlpha = 255;
        int maxAlpha = 0;
        const QImage mask = result->mask.convertToFormat(QImage::Format_Grayscale8);
        for (int y = 0; y < mask.height(); ++y) {
            const uchar* line = mask.constScanLine(y);
            for (int x = 0; x < mask.width(); ++x) {
                minAlpha = std::min(minAlpha, static_cast<int>(line[x]));
                maxAlpha = std::max(maxAlpha, static_cast<int>(line[x]));
            }
        }

        QVERIFY(maxAlpha > minAlpha);
    }
};

int runIntegrationTests(int argc, char** argv) {
    IntegrationTests tests;
    return QTest::qExec(&tests, argc, argv);
}

}  // namespace rembg::native_app

#include "IntegrationTests.moc"
