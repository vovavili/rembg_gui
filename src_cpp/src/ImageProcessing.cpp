#include "ImageProcessing.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <QColor>

namespace rembg::native_app {

Expected<ImageTensor> makeU2NetInput(const QImage& source, QSize inputSize) {
    if (source.isNull()) {
        return failure("Cannot process an empty image.");
    }
    if (!inputSize.isValid() || inputSize.isEmpty()) {
        return failure("The model input size is invalid.");
    }

    const QImage rgb = source.convertToFormat(QImage::Format_RGB888)
                           .scaled(inputSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    float maxPixel = 0.0F;
    for (int y = 0; y < rgb.height(); ++y) {
        const uchar* line = rgb.constScanLine(y);
        for (int x = 0; x < rgb.width() * 3; ++x) {
            maxPixel = std::max(maxPixel, static_cast<float>(line[x]));
        }
    }

    const float denominator = std::max(maxPixel, 1.0e-6F);
    constexpr std::array<float, 3> means{0.485F, 0.456F, 0.406F};
    constexpr std::array<float, 3> stddevs{0.229F, 0.224F, 0.225F};
    const int pixelCount = rgb.width() * rgb.height();

    ImageTensor tensor;
    tensor.size = inputSize;
    tensor.values.resize(static_cast<size_t>(pixelCount * 3));

    for (int y = 0; y < rgb.height(); ++y) {
        const uchar* line = rgb.constScanLine(y);
        for (int x = 0; x < rgb.width(); ++x) {
            const int pixelOffset = y * rgb.width() + x;
            for (int channel = 0; channel < 3; ++channel) {
                const float normalized =
                    (static_cast<float>(line[x * 3 + channel]) / denominator - means[channel]) /
                    stddevs[channel];
                tensor.values[static_cast<size_t>(channel * pixelCount + pixelOffset)] =
                    normalized;
            }
        }
    }

    return tensor;
}

Expected<QImage> maskFromPrediction(std::span<const float> prediction, QSize predictionSize) {
    if (!predictionSize.isValid() || predictionSize.isEmpty()) {
        return failure("The prediction size is invalid.");
    }

    const size_t expectedSize =
        static_cast<size_t>(predictionSize.width() * predictionSize.height());
    if (prediction.size() < expectedSize) {
        return failure("The prediction tensor does not contain enough mask values.");
    }

    const auto [minIt, maxIt] =
        std::minmax_element(prediction.begin(), prediction.begin() + expectedSize);
    if (minIt == prediction.end() || maxIt == prediction.end()) {
        return failure("The prediction tensor is empty.");
    }

    const float minValue = *minIt;
    const float maxValue = *maxIt;
    const float range = maxValue - minValue;

    QImage mask(predictionSize, QImage::Format_Grayscale8);
    if (mask.isNull()) {
        return failure("Failed to allocate the output mask.");
    }

    for (int y = 0; y < predictionSize.height(); ++y) {
        uchar* line = mask.scanLine(y);
        for (int x = 0; x < predictionSize.width(); ++x) {
            const size_t index = static_cast<size_t>(y * predictionSize.width() + x);
            float normalized = 0.0F;
            if (range > std::numeric_limits<float>::epsilon()) {
                normalized = (prediction[index] - minValue) / range;
            }
            line[x] = static_cast<uchar>(
                std::clamp(std::round(normalized * 255.0F), 0.0F, 255.0F)
            );
        }
    }

    return mask;
}

Expected<QImage> applyMaskAsAlpha(const QImage& source, const QImage& mask) {
    if (source.isNull()) {
        return failure("Cannot apply a mask to an empty image.");
    }
    if (mask.isNull()) {
        return failure("Cannot apply an empty mask.");
    }
    if (source.size() != mask.size()) {
        return failure("The image and mask sizes do not match.");
    }

    QImage output = source.convertToFormat(QImage::Format_ARGB32);
    const QImage alphaMask = mask.convertToFormat(QImage::Format_Grayscale8);

    for (int y = 0; y < output.height(); ++y) {
        QRgb* outputLine = reinterpret_cast<QRgb*>(output.scanLine(y));
        const uchar* maskLine = alphaMask.constScanLine(y);

        for (int x = 0; x < output.width(); ++x) {
            const QRgb pixel = outputLine[x];
            outputLine[x] = qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), maskLine[x]);
        }
    }

    return output;
}

}  // namespace rembg::native_app
