#include "ImageProcessing.h"
#include "ModelManager.h"
#include "PathUtils.h"

#include <array>
#include <cmath>
#include <filesystem>

#include <QByteArray>
#include <QColor>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace rembg::native_app {

class ImageProcessingTests : public QObject {
    Q_OBJECT

private slots:
    void u2netPreprocessUsesChannelFirstNormalizedFloats() {
        QImage image(1, 1, QImage::Format_RGB888);
        image.setPixelColor(0, 0, QColor(10, 20, 40));

        auto tensor = makeU2NetInput(image, QSize(2, 2));
        QVERIFY(tensor.has_value());
        QCOMPARE(tensor->values.size(), static_cast<size_t>(12));

        const float red = (10.0F / 40.0F - 0.485F) / 0.229F;
        const float green = (20.0F / 40.0F - 0.456F) / 0.224F;
        const float blue = (40.0F / 40.0F - 0.406F) / 0.225F;

        QVERIFY(std::abs(tensor->values[0] - red) < 0.0001F);
        QVERIFY(std::abs(tensor->values[4] - green) < 0.0001F);
        QVERIFY(std::abs(tensor->values[8] - blue) < 0.0001F);
    }

    void maskPredictionIsMinMaxNormalized() {
        const std::array<float, 4> prediction{2.0F, 3.0F, 4.0F, 4.0F};
        auto mask = maskFromPrediction(prediction, QSize(2, 2));
        QVERIFY(mask.has_value());

        QCOMPARE(qGray(mask->pixel(0, 0)), 0);
        QCOMPARE(qGray(mask->pixel(1, 0)), 128);
        QCOMPARE(qGray(mask->pixel(0, 1)), 255);
        QCOMPARE(qGray(mask->pixel(1, 1)), 255);
    }

    void alphaMaskIsAppliedToSourceImage() {
        QImage source(2, 1, QImage::Format_RGB888);
        source.setPixelColor(0, 0, QColor(255, 0, 0));
        source.setPixelColor(1, 0, QColor(0, 0, 255));

        QImage mask(2, 1, QImage::Format_Grayscale8);
        mask.scanLine(0)[0] = 0;
        mask.scanLine(0)[1] = 255;

        auto output = applyMaskAsAlpha(source, mask);
        QVERIFY(output.has_value());

        QCOMPARE(qAlpha(output->pixel(0, 0)), 0);
        QCOMPARE(qAlpha(output->pixel(1, 0)), 255);
        QCOMPARE(qRed(output->pixel(0, 0)), 255);
        QCOMPARE(qBlue(output->pixel(1, 0)), 255);
    }

    void md5VerificationMatchesKnownDigest() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString filePath = directory.filePath("payload.txt");
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("abc"), qint64{3});
        file.close();

        QVERIFY(ModelManager::hasExpectedMd5(qStringToPath(filePath), "900150983cd24fb0d6963f7d28e17f72"));
    }

    void modelDirectoryHonorsU2NetHome() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const bool hadPreviousValue = qEnvironmentVariableIsSet("U2NET_HOME");
        const QByteArray previousValue = qgetenv("U2NET_HOME");
        qputenv("U2NET_HOME", directory.path().toUtf8());

        QVERIFY(ModelManager::modelDirectory() == qStringToPath(directory.path()));

        if (hadPreviousValue) {
            qputenv("U2NET_HOME", previousValue);
        } else {
            qunsetenv("U2NET_HOME");
        }
    }
};

int runImageProcessingTests(int argc, char** argv) {
    ImageProcessingTests tests;
    return QTest::qExec(&tests, argc, argv);
}

}  // namespace rembg::native_app

#include "ImageProcessingTests.moc"
