#ifndef LOGIKSIM_LAYOUT_H
#define LOGIKSIM_LAYOUT_H

#include "line_tree.h"
#include "vocabulary.h"

#include <fmt/core.h>

#include <cstdint>
#include <string>
#include <vector>

namespace logicsim {

enum class DisplayState : uint8_t {
    normal,

    new_valid,
    new_colliding,

    new_temporary,
};

auto format(DisplayState state) -> std::string;

class Layout {
   public:
    [[nodiscard]] explicit Layout() = default;
    [[nodiscard]] explicit Layout(circuit_id_t circuit_id);

    auto swap(Layout &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_element(element_id_t element_id) const -> std::string;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto element_count() const -> std::size_t;

    // TODO make add_default_element private when not needed anymore
    auto add_default_element() -> element_id_t;

    auto add_placeholder() -> element_id_t;
    auto add_wire(LineTree &&line_tree) -> element_id_t;
    auto add_logic_element(point_t position,
                           orientation_t orientation = orientation_t::undirected,
                           DisplayState display_state = DisplayState::normal,
                           color_t color = defaults::color_black) -> element_id_t;
    // swaps the element with last one and deletes it
    auto swap_and_delete_element(element_id_t element_id) -> element_id_t;

    // TODO remove line tree, when not needed anymore
    auto set_line_tree(element_id_t element_id, LineTree &&line_tree) -> void;
    auto set_position(element_id_t element_id, point_t point) -> void;
    auto set_display_state(element_id_t element_id, DisplayState display_state) -> void;

    [[nodiscard]] auto circuit_id() const noexcept -> circuit_id_t;
    [[nodiscard]] auto line_tree(element_id_t element_id) const -> const LineTree &;
    [[nodiscard]] auto position(element_id_t element_id) const -> point_t;
    [[nodiscard]] auto orientation(element_id_t element_id) const -> orientation_t;
    [[nodiscard]] auto display_state(element_id_t element_id) const -> DisplayState;
    [[nodiscard]] auto color(element_id_t element_id) const -> color_t;

   private:
    auto swap_element_data(element_id_t element_id_1, element_id_t element_id_2) -> void;
    auto delete_last_element() -> void;

    std::vector<LineTree> line_trees_ {};
    std::vector<point_t> positions_ {};
    std::vector<orientation_t> orientation_ {};
    std::vector<DisplayState> display_states_ {};
    std::vector<color_t> colors_ {};

    circuit_id_t circuit_id_ {0};
};

auto swap(Layout &a, Layout &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Layout &a, logicsim::Layout &b) noexcept -> void;

template <>
struct fmt::formatter<logicsim::DisplayState> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::DisplayState &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

template <>
struct fmt::formatter<logicsim::orientation_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::orientation_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

#endif