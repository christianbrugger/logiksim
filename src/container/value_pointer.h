#ifndef LOGICSIM_CONTAINER_VALUE_POINTER_H
#define LOGICSIM_CONTAINER_VALUE_POINTER_H

#include "type_trait/is_equality_comparable.h"

#include <fmt/core.h>

#include <any>
#include <compare>
#include <memory>
#include <type_traits>
#include <utility>

namespace logicsim {

// similar to std::optional requirements
template <class T>
concept is_value_pointer_compatible =
    !std::is_same_v<std::remove_cv_t<T>, std::in_place_t> &&  //
    std::is_object_v<T> && std::is_destructible_v<T> && !std::is_array_v<T>;

/**
 * @brief: Value like type for incomplete types stored on the heap.
 *
 * This class is suitable to implement the Pimpl idiom. For this add a explicit
 * template instantiation of value_pointer<incomplete_t> to the h and cpp file.
 *
 *      header: extern template class value_pointer<incomplete_t, ..>;
 *      cpp:    template class value_pointer<incomplete_t, ..>;
 *
 * Template arguments:
 *      T:  The possibly incomplete type that is held.
 *      C:  When set to std::true_type the equality operator is enabled.
 *      O:  To support three-way-comparison O needs to be set to one of
 *          std::strong_ordering, std::partial_ordering, std::weak_ordering.
 *
 */
template <typename T, typename C = std::false_type, typename O = void>
class value_pointer {
   public:
    using value_type = T;

    [[nodiscard]] explicit value_pointer();

    [[nodiscard]] explicit value_pointer(const T& value);
    [[nodiscard]] explicit value_pointer(T&& value);

    template <class... Args>
    [[nodiscard]] explicit value_pointer(std::in_place_t /*unused*/, Args&&... args)
        requires std::is_constructible_v<T, Args...>;

    // defined by unique ptr
    ~value_pointer() noexcept;
    [[nodiscard]] value_pointer(value_pointer&& other) noexcept;
    auto operator=(value_pointer&& other) noexcept -> value_pointer&;

    // add copy operations
    [[nodiscard]] value_pointer(const value_pointer& other);
    auto operator=(const value_pointer& other) -> value_pointer&;

    template <typename T_, typename C_, typename O_>
    friend auto swap(value_pointer<T_, C_, O_>& a, value_pointer<T_, C_, O_>& b) noexcept
        -> void;

    // const preserving access
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

    // comparison as members so they are auto-generated for template instantiations
    [[nodiscard]] auto operator==(const value_pointer& other) const -> bool
        requires(std::same_as<C, std::true_type>);
    [[nodiscard]] auto operator<=>(const value_pointer& other) const -> O
        requires(!std::is_void_v<O>);

   private:
    std::unique_ptr<T> value_;
};

/**
 * @brief: Value pointer for complete types that automatically sets comparison and
 *         ordering template parameters.
 */
template <typename T>
using value_pointer_complete =
    value_pointer<T, is_equality_comparable_v<T>, std::compare_three_way_result_t<T, T>>;

//
// Implementation
//

template <typename T, typename C, typename O>
value_pointer<T, C, O>::value_pointer() : value_ {std::make_unique<value_type>()} {
    static_assert(is_value_pointer_compatible<T>);
}

template <typename T, typename C, typename O>
value_pointer<T, C, O>::value_pointer(const T& value)
    : value_ {std::make_unique<value_type>(value)} {
    static_assert(is_value_pointer_compatible<T>);
}

template <typename T, typename C, typename O>
value_pointer<T, C, O>::value_pointer(T&& value)
    : value_ {std::make_unique<value_type>(std::move(value))} {
    static_assert(is_value_pointer_compatible<T>);
}

template <typename T, typename C, typename O>
template <class... Args>
value_pointer<T, C, O>::value_pointer(std::in_place_t /*unused*/, Args&&... args)
    requires std::is_constructible_v<T, Args...>
    : value_ {std::make_unique<value_type>(std::forward<Args>(args)...)} {
    static_assert(is_value_pointer_compatible<T>);
}

// special members

template <typename T, typename C, typename O>
value_pointer<T, C, O>::~value_pointer() noexcept = default;

template <typename T, typename C, typename O>
value_pointer<T, C, O>::value_pointer(value_pointer&& other) noexcept = default;

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator=(value_pointer&& other) noexcept
    -> value_pointer& = default;

template <typename T, typename C, typename O>
value_pointer<T, C, O>::value_pointer(const value_pointer& other)
    : value_ {std::make_unique<value_type>(*other.value_)} {}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator=(const value_pointer& other) -> value_pointer& {
    auto tmp = value_pointer {other};
    swap(*this, tmp);
    return *this;
}

template <typename T, typename C, typename O>
auto swap(value_pointer<T, C, O>& a, value_pointer<T, C, O>& b) noexcept -> void {
    using std::swap;
    swap(a.value_, b.value_);
}

// access operators

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator->() const noexcept -> const T* {
    return value_.get();
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator->() noexcept -> T* {
    return value_.get();
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator*() const& noexcept -> const T& {
    return *value_;
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator*() & noexcept -> T& {
    return *value_;
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator*() const&& noexcept -> const T&& {
    return std::move(*value_);
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator*() && noexcept -> T&& {
    return std::move(*value_);
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::value() const& noexcept -> const T& {
    return *value_;
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::value() & noexcept -> T& {
    return *value_;
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::value() const&& noexcept -> const T&& {
    return std::move(*value_);
}

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::value() && noexcept -> T&& {
    return std::move(*value_);
}

// comparison

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator==(const value_pointer& other) const -> bool
    requires(std::same_as<C, std::true_type>)
{
    return value() == other.value();
};

template <typename T, typename C, typename O>
auto value_pointer<T, C, O>::operator<=>(const value_pointer& other) const -> O
    requires(!std::is_void_v<O>)
{
    static_assert(
        std::same_as<std::compare_three_way_result_t<T, T>, std::strong_ordering>);
    return value() <=> other.value();
};

}  // namespace logicsim

template <typename T, typename C, typename O, typename Char>
struct fmt::formatter<logicsim::value_pointer<T, C, O>, Char> {
    constexpr auto parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }

    inline auto format(const logicsim::value_pointer<T, C, O>& obj,
                       fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.value());
    }
};

#endif
