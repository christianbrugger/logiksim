#ifndef LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_KEY_STATE_H
#define LOGICSIM_CORE_COMPONENT_EDITABLE_CIRCUIT_KEY_STATE_H

#include "core/layout.h"
#include "core/vocabulary/segment_key.h"

namespace logicsim {

namespace editable_circuit {

class Modifier;

/**
 * @brief: Uniquely identify one key entry for segments
 */
struct key_state_entry_t {
    segment_key_t key;
    ordered_line_t line;
    std::pair<display_state_t, display_state_t> display_states;

    [[nodiscard]] auto operator==(const key_state_entry_t &) const -> bool = default;
    [[nodiscard]] auto operator<=>(const key_state_entry_t &) const = default;
    [[nodiscard]] auto format() const -> std::string;
};

using key_state_t = std::vector<key_state_entry_t>;

[[nodiscard]] auto get_sorted_key_state(const Modifier &modifier) -> key_state_t;

/**
 * @brief: State of layout and keys that is comparable.
 */

struct layout_key_state_t {
    [[nodiscard]] explicit layout_key_state_t() = default;
    [[nodiscard]] explicit layout_key_state_t(const Modifier &modifier);

    [[nodiscard]] auto operator==(const layout_key_state_t &) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

   private:
    Layout normalized_layout_;
    key_state_t sorted_key_state_;
};

static_assert(std::regular<layout_key_state_t>);

}  // namespace editable_circuit

}  // namespace logicsim

#endif
