#pragma once

#include <filesystem>
#include <string>

#include <QByteArray>
#include <QString>

namespace rembg::native_app {

inline QString pathToQString(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    const std::string nativePath = path.string();
    return QString::fromLocal8Bit(nativePath.c_str());
#endif
}

inline std::filesystem::path qStringToPath(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    const QByteArray nativePath = path.toLocal8Bit();
    return std::filesystem::path(nativePath.constData());
#endif
}

}  // namespace rembg::native_app
