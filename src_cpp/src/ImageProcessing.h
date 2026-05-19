#pragma once

#include "Expected.h"

#include <span>
#include <vector>

#include <QImage>
#include <QSize>

namespace rembg::native_app {

struct ImageTensor {
    std::vector<float> values;
    QSize size;
};

struct RemovalResult {
    QImage image;
    QImage mask;
};

Expected<ImageTensor> makeU2NetInput(const QImage& source, QSize inputSize);
Expected<QImage> maskFromPrediction(std::span<const float> prediction, QSize predictionSize);
Expected<QImage> applyMaskAsAlpha(const QImage& source, const QImage& mask);

}  // namespace rembg::native_app
