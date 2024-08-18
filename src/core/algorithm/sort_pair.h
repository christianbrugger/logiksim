#ifndef LOGICSIM_ALGORITHM_SORT_PAIR_H
#define LOGICSIM_ALGORITHM_SORT_PAIR_H

#include <functional>
#include <ranges>
#include <tuple>
#include <utility>

namespace logicsim {

/**
 * brief: Sort two given values in-place.
 */
template <class T, class Comp = std::ranges::less>
constexpr auto sort_inplace(T& a, T& b,
                            Comp comp = {}) noexcept(noexcept(comp(a, b))) -> void {
    if (!std::invoke(comp, a, b)) {
        using std::swap;
        swap(a, b);
    }
}

/**
 * brief: Get sorted references.
 */
template <class T, class Comp = std::ranges::less>
[[nodiscard]] constexpr auto sorted_ref(T& a, T& b, Comp comp = {}) noexcept(
    noexcept(comp(a, b))) -> std::tuple<T&, T&> {
    if (std::invoke(comp, a, b)) {
        return std::tie(a, b);
    }
    return std::tie(b, a);
}

/**
 * brief: Get sorted values (copied).
 */
template <class T>
[[nodiscard]] constexpr auto sorted(T&& a, T&& b) noexcept(noexcept(a <= b))
    -> std::tuple<std::remove_cvref_t<T>, std::remove_cvref_t<T>> {
    if (a <= b) {
        return std::make_tuple(std::forward<T>(a), std::forward<T>(b));
    }
    return std::make_tuple(std::forward<T>(b), std::forward<T>(a));
}

}  // namespace logicsim

#endif
