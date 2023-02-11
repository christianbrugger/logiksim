
#include "circuit_layout.h"

#include "exceptions.h"

namespace logicsim {

auto format(DisplayState state) -> std::string {
    switch (state) {
        using enum DisplayState;

        case normal:
            return "Normal";
        case selected:
            return "Selected";
        case new_unknown:
            return "NewUnknown";
        case new_valid:
            return "NewValid";
        case new_colliding:
            return "NewColliding";
    }
    throw_exception("Don't know how to convert ElementType to string.");
}

CircuitLayout::CircuitLayout(circuit_id_t circuit_id) : circuit_id_ {circuit_id} {
    if (circuit_id < circuit_id_t {0}) [[unlikely]] {
        throw_exception("Circuit id of layout cannot be negative.");
    }
}

auto CircuitLayout::format() const -> std::string {
    return "CircuitLayout( ... )";
}

auto CircuitLayout::add_default_element() -> void {
    line_trees_.push_back(LineTree {});
    positions_.push_back(point_t {});
    orientation_.push_back(DisplayOrientation::default_right);
    display_states_.push_back(DisplayState::normal);
    colors_.push_back(defaults::color_black);
}

auto CircuitLayout::add_line_tree(LineTree &&line_tree) -> void {
    add_default_element();
    line_trees_.back() = std::move(line_tree);
}

auto CircuitLayout::add_simple_element(point_t position, DisplayOrientation orientation,
                                       DisplayState display_state, color_t color)
    -> void {
    add_default_element();
    positions_.back() = position;
    orientation_.back() = orientation;
    display_states_.back() = display_state;
    colors_.back() = color;
}

auto CircuitLayout::set_line_tree(element_id_t element_id, LineTree &&line_tree) -> void {
    line_trees_.at(element_id.value) = std::move(line_tree);
}

auto CircuitLayout::set_position(element_id_t element_id, point_t position) -> void {
    positions_.at(element_id.value) = position;
}

auto CircuitLayout::circuit_id() const noexcept -> circuit_id_t {
    return circuit_id_;
}

auto CircuitLayout::line_tree(element_id_t element_id) const -> const LineTree & {
    return line_trees_.at(element_id.value);
}

auto CircuitLayout::position(element_id_t element_id) const -> point_t {
    return positions_.at(element_id.value);
}

auto CircuitLayout::orientation(element_id_t element_id) const -> DisplayOrientation {
    return orientation_.at(element_id.value);
}

auto CircuitLayout::display_state(element_id_t element_id) const -> DisplayState {
    return display_states_.at(element_id.value);
}

auto CircuitLayout::color(element_id_t element_id) const -> color_t {
    return colors_.at(element_id.value);
}

}  // namespace logicsim
