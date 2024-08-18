#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_MIN_INDEX_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_MIN_INDEX_H

#include "component/simulation/history_index.h"

#include <compare>

namespace logicsim {

namespace simulation {

/**
 * @brief: Lowest index that is valid for the history-buffer.
 *
 * Class invariants:
 *      * index cannot be negative
 */
struct history_min_index_t {
    history_index_t index {};

    [[nodiscard]] explicit constexpr history_min_index_t() = default;
    [[nodiscard]] explicit constexpr history_min_index_t(history_index_t index);

    [[nodiscard]] explicit constexpr operator std::size_t() const;
    [[nodiscard]] explicit constexpr operator std::ptrdiff_t() const;
    [[nodiscard]] explicit constexpr operator history_index_t() const;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const history_min_index_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const history_min_index_t &other) const =
        default;
};

[[nodiscard]] constexpr auto operator==(const history_min_index_t &left,
                                        const history_index_t &right) -> bool;
[[nodiscard]] constexpr auto operator<=>(const history_min_index_t &left,
                                         const history_index_t &right);
// symmetric
[[nodiscard]] constexpr auto operator==(const history_index_t &left,
                                        const history_min_index_t &right) -> bool;
[[nodiscard]] constexpr auto operator<=>(const history_index_t &left,
                                         const history_min_index_t &right);

//
// Implementation
//

constexpr history_min_index_t::history_min_index_t(history_index_t index_)
    : index {index_} {
    if (index < history_index_t {0}) [[unlikely]] {
        throw std::runtime_error("min index needs to be positive");
    }
}

constexpr history_min_index_t::operator std::size_t() const {
    assert(index.value >= 0);
    return static_cast<std::size_t>(index.value);
}

constexpr history_min_index_t::operator std::ptrdiff_t() const {
    return std::ptrdiff_t {index};
}

constexpr history_min_index_t::operator history_index_t() const {
    return index;
}

//
// Free Functions
//

constexpr auto operator==(const history_min_index_t &left,
                          const history_index_t &right) -> bool {
    return left.index == right;
}

constexpr auto operator<=>(const history_min_index_t &left,
                           const history_index_t &right) {
    return left.index <=> right;
}

constexpr auto operator==(const history_index_t &left,
                          const history_min_index_t &right) -> bool {
    return left == right.index;
}

constexpr auto operator<=>(const history_index_t &left,
                           const history_min_index_t &right) {
    return left <=> right.index;
}

}  // namespace simulation

}  // namespace logicsim

#endif
