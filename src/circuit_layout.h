#ifndef LOGIKSIM_CIRCUIT_LAYOUT_H
#define LOGIKSIM_CIRCUIT_LAYOUT_H

#include "line_tree.h"
#include "vocabulary.h"

#include <fmt/core.h>

#include <cstdint>
#include <string>
#include <vector>

namespace logicsim {

enum class DisplayState : uint8_t {
    normal,
    selected,

    new_unknown,
    new_valid,
    new_colliding,
};

auto format(DisplayState state) -> std::string;

enum class DisplayOrientation : uint8_t {
    default_right,
};

// TODO rename to Layout
class CircuitLayout {
   public:
    [[nodiscard]] explicit CircuitLayout() = default;
    [[nodiscard]] explicit CircuitLayout(circuit_id_t circuit_id);

    auto swap(CircuitLayout &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;

    auto add_default_element() -> void;
    auto add_wire(LineTree &&line_tree) -> void;
    auto add_logic_element(point_t position,
                           DisplayOrientation orientation
                           = DisplayOrientation::default_right,
                           DisplayState display_state = DisplayState::normal,
                           color_t color = defaults::color_black) -> void;

    // TODO remove these, when not needed anymore
    auto set_line_tree(element_id_t element_id, LineTree &&line_tree) -> void;
    auto set_position(element_id_t element_id, point_t point) -> void;

    [[nodiscard]] auto circuit_id() const noexcept -> circuit_id_t;
    [[nodiscard]] auto line_tree(element_id_t element_id) const -> const LineTree &;
    [[nodiscard]] auto position(element_id_t element_id) const -> point_t;
    [[nodiscard]] auto orientation(element_id_t element_id) const -> DisplayOrientation;
    [[nodiscard]] auto display_state(element_id_t element_id) const -> DisplayState;
    [[nodiscard]] auto color(element_id_t element_id) const -> color_t;

   private:
    std::vector<LineTree> line_trees_ {};
    std::vector<point_t> positions_ {};
    std::vector<DisplayOrientation> orientation_ {};
    std::vector<DisplayState> display_states_ {};
    std::vector<color_t> colors_ {};

    circuit_id_t circuit_id_ {0};
};

auto swap(CircuitLayout &a, CircuitLayout &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::CircuitLayout &a, logicsim::CircuitLayout &b) noexcept -> void;

// TODO make generic formatter
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
struct fmt::formatter<logicsim::CircuitLayout> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::CircuitLayout &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

#endif