#include "core/component/editable_circuit/key_state.h"

#include "core/component/editable_circuit/modifier.h"

namespace logicsim {

namespace editable_circuit {

auto key_state_entry_t::format() const -> std::string {
    return fmt::format("({}, {}, {})", key, line, display_states);
}

auto get_sorted_key_state(const Modifier &modifier) -> key_state_t {
    const auto &layout = modifier.circuit_data().layout;
    auto result = key_state_t {};

    for (const auto wire_id : wire_ids(layout)) {
        for (const segment_t segment :
             layout.wires().segment_tree(wire_id).indices(wire_id)) {
            const auto segment_part = get_segment_part(layout, segment);
            result.emplace_back(modifier.circuit_data().index.key_index().get(segment),
                                get_line(layout, segment),
                                get_display_states(layout, segment_part));
        }
    }

    std::ranges::sort(result);
    return result;
}

auto layout_key_state_t::format() const -> std::string {
    return fmt::format(
        "layout_key_state(\n"
        "  {}\n"
        "  key_state = {},\n"
        ")",
        normalized_layout_, sorted_key_state_);
}

layout_key_state_t::layout_key_state_t(const Modifier &modifier)
    : normalized_layout_ {get_normalized(modifier.circuit_data().layout)},
      sorted_key_state_ {get_sorted_key_state(modifier)} {}

}  // namespace editable_circuit

}  // namespace logicsim
