#ifndef LOGICSIM_CONTAINER_VALUE_POINTER_H
#define LOGICSIM_CONTAINER_VALUE_POINTER_H

#include <fmt/core.h>

#include <format/struct.h>
#include <memory>
#include <type_traits>
#include <utility>

// using a = std::optional<int>;

namespace logicsim {

template <typename T>
class value_pointer {
   public:
    static_assert(!std::is_same_v<std::remove_cv_t<T>, std::in_place_t>,
                  "T in value_pointer<T> must be a type other than in_place_t.");
    static_assert(std::is_object_v<T> && std::is_destructible_v<T> && !std::is_array_v<T>,
                  "T in optional<T> must meet the destructible requirements.");

    using value_type = T;

    [[nodiscard]] explicit value_pointer();

    template <class... Args>
    [[nodiscard]] explicit value_pointer(std::in_place_t, Args&&... args)
        requires std::is_constructible_v<T, Args...>;

    [[nodiscard]] explicit value_pointer(const T& value);
    [[nodiscard]] explicit value_pointer(T&& value);

    // defined by unique ptr
    constexpr ~value_pointer() = default;
    [[nodiscard]] value_pointer(value_pointer&& other) noexcept = default;
    auto operator=(value_pointer&& other) noexcept -> value_pointer& = default;

    // add copy operations
    [[nodiscard]] value_pointer(const value_pointer& other);
    auto operator=(const value_pointer& other) -> value_pointer&;

    template <class U>
    friend auto swap(value_pointer<U>& a, value_pointer<U>& b) noexcept -> void;

    // const-preserving access
    [[nodiscard]] auto operator->() const noexcept -> const T*;
    [[nodiscard]] auto operator->() noexcept -> T*;

    [[nodiscard]] auto operator*() const& noexcept -> const T&;
    [[nodiscard]] auto operator*() & noexcept -> T&;

    [[nodiscard]] auto operator*() const&& noexcept -> const T&&;
    [[nodiscard]] auto operator*() && noexcept -> T&&;

    [[nodiscard]] auto value() const& noexcept -> const T&;
    [[nodiscard]] auto value() & noexcept -> T&;

    [[nodiscard]] auto value() const&& noexcept -> const T&&;
    [[nodiscard]] auto value() && noexcept -> T&&;

   private:
    std::unique_ptr<T> value_;
};

//
// Free Functions
//

// template <class T1, std::equality_comparable_with<T1> T2>
//[[nodiscard]] constexpr auto operator==(const value_pointer<T1>& left,
//                                         const T2& right) noexcept(noexcept(bool {
//     *left == right})) -> bool;

template <class T1, std::equality_comparable_with<T1> T2>
[[nodiscard]] constexpr auto operator==(
    const value_pointer<T1>& left,
    const value_pointer<T2>& right) noexcept(noexcept(bool {*left == *right})) -> bool;

template <class T1, std::three_way_comparable_with<T1> T2>
[[nodiscard]] constexpr auto operator<=>(
    const value_pointer<T1>& left,
    const value_pointer<T2>& right) noexcept(noexcept(*left <=> *right))
    -> std::compare_three_way_result_t<T1, T2>;

//

// TODO make_value

//
// Implementation
//

template <typename T>
value_pointer<T>::value_pointer() : value_ {std::make_unique<value_type>()} {}

template <typename T>
template <class... Args>
value_pointer<T>::value_pointer(std::in_place_t, Args&&... args)
    requires std::is_constructible_v<T, Args...>
    : value_ {std::make_unique<value_type>(std::forward<Args>(args)...)} {}

template <typename T>
value_pointer<T>::value_pointer(const T& value)
    : value_ {std::make_unique<value_type>(value)} {}

template <typename T>
value_pointer<T>::value_pointer(T&& value)
    : value_ {std::make_unique<value_type>(std::move(value))} {}

// special members

template <typename T>
value_pointer<T>::value_pointer(const value_pointer& other)
    : value_ {std::make_unique<value_type>(*(other.value_))} {}

template <typename T>
auto value_pointer<T>::operator=(const value_pointer& other) -> value_pointer& {
    auto tmp = value_pointer {other};
    swap(*this, tmp);
    return *this;
}

template <typename U>
auto swap(value_pointer<U>& a, value_pointer<U>& b) noexcept -> void {
    using std::swap;
    swap(a.value_, b.value_);
}

// access operators

template <typename T>
auto value_pointer<T>::operator->() const noexcept -> const T* {
    return value_.get();
}

template <typename T>
auto value_pointer<T>::operator->() noexcept -> T* {
    return value_.get();
}

template <typename T>
auto value_pointer<T>::operator*() const& noexcept -> const T& {
    return *value_;
}

template <typename T>
auto value_pointer<T>::operator*() & noexcept -> T& {
    return *value_;
}

template <typename T>
auto value_pointer<T>::operator*() const&& noexcept -> const T&& {
    return std::move(*value_);
}

template <typename T>
auto value_pointer<T>::operator*() && noexcept -> T&& {
    return std::move(*value_);
}

template <typename T>
auto value_pointer<T>::value() const& noexcept -> const T& {
    return *value_;
}

template <typename T>
auto value_pointer<T>::value() & noexcept -> T& {
    return *value_;
}

template <typename T>
auto value_pointer<T>::value() const&& noexcept -> const T&& {
    return std::move(*value_);
}

template <typename T>
auto value_pointer<T>::value() && noexcept -> T&& {
    return std::move(*value_);
}

//
// Free Functions
//

// template <class T1, std::equality_comparable_with<T1> T2>
// constexpr auto operator==(const value_pointer<T1>& left,
//                           const T2& right) noexcept(noexcept(bool {*left == right}))
//     -> bool {
//     return *left == right;
// }

template <class T1, std::equality_comparable_with<T1> T2>
constexpr auto operator==(const value_pointer<T1>& left,
                          const value_pointer<T2>& right) noexcept(noexcept(bool {
    *left == *right})) -> bool {
    return *left == *right;
}

template <class T1, std::three_way_comparable_with<T1> T2>
constexpr auto operator<=>(const value_pointer<T1>& left,
                           const value_pointer<T2>& right) noexcept(noexcept(*left <=>
                                                                             *right))
    -> std::compare_three_way_result_t<T1, T2> {
    return *left <=> *right;
}

}  // namespace logicsim

template <typename T, typename Char>
struct fmt::formatter<logicsim::value_pointer<T>, Char> {
    constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    inline auto format(const logicsim::value_pointer<T>& obj,
                       fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.value());
    }
};

#endif
