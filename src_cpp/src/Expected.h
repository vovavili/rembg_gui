#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include <QString>

namespace rembg::native_app {

class Failure {
public:
    explicit Failure(QString message)
        : message_(std::move(message)) {}

private:
    template <class T>
    friend class Expected;

    QString message_;
};

template <class T>
class Expected {
public:
    template <class U>
        requires(
            !std::is_same_v<std::remove_cvref_t<U>, Expected>
            && !std::is_same_v<std::remove_cvref_t<U>, Failure>
            && std::is_constructible_v<T, U&&>
        )
    Expected(U&& value)
        : storage_(std::in_place_index<0>, std::forward<U>(value)) {}

    Expected(Failure failure)
        : storage_(std::in_place_index<1>, std::move(failure.message_)) {}

    Expected(const Expected&) = default;
    Expected(Expected&&) = default;
    Expected& operator=(const Expected&) = default;
    Expected& operator=(Expected&&) = default;
    ~Expected() = default;

    [[nodiscard]] bool has_value() const noexcept { return storage_.index() == 0; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] T& value() & { return std::get<0>(storage_); }
    [[nodiscard]] const T& value() const& { return std::get<0>(storage_); }
    [[nodiscard]] T&& value() && { return std::move(std::get<0>(storage_)); }

    [[nodiscard]] T& operator*() & { return value(); }
    [[nodiscard]] const T& operator*() const& { return value(); }
    [[nodiscard]] T&& operator*() && { return std::move(*this).value(); }

    [[nodiscard]] T* operator->() { return std::addressof(value()); }
    [[nodiscard]] const T* operator->() const { return std::addressof(value()); }

    [[nodiscard]] const QString& error() const& { return std::get<1>(storage_); }
    [[nodiscard]] QString&& error() && { return std::move(std::get<1>(storage_)); }

private:
    std::variant<T, QString> storage_;
};

inline Failure failure(QString message) {
    return Failure(std::move(message));
}

inline Failure failure(const char* message) {
    return failure(QString::fromUtf8(message));
}

}  // namespace rembg::native_app
