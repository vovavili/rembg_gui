#include "BackgroundRemovalEngine.h"
#include "ModelManager.h"

#include <algorithm>
#include <exception>

#include <QImage>
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
    void runEnabledModelTest() {
        QImage image(QString::fromUtf8(REMBG_NATIVE_SAMPLE_IMAGE));
        QVERIFY2(!image.isNull(), "The sample image could not be loaded.");

        auto modelPath = ModelManager::ensureU2NetModel();
        if (!modelPath.has_value()) {
            const QByteArray message = modelPath.error().toUtf8();
            QFAIL(message.constData());
        }

        auto engine = BackgroundRemovalEngine::create(*modelPath);
        if (!engine.has_value()) {
            const QByteArray message = engine.error().toUtf8();
            QFAIL(message.constData());
        }

        auto result = (*engine)->removeBackground(image);
        if (!result.has_value()) {
            const QByteArray message = result.error().toUtf8();
            QFAIL(message.constData());
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
