#pragma once

#include <expected>
#include <utility>

#include <QString>

namespace rembg::native_app {

template <class T>
using Expected = std::expected<T, QString>;

inline std::unexpected<QString> failure(QString message) {
    return std::unexpected<QString>(std::move(message));
}

inline std::unexpected<QString> failure(const char* message) {
    return failure(QString::fromUtf8(message));
}

}  // namespace rembg::native_app
