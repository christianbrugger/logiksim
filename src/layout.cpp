
#include "layout.h"

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
    throw_exception("Don't know how to convert DisplayState to string.");
}

auto format(DisplayOrientation orientation) -> std::string {
    switch (orientation) {
        using enum DisplayOrientation;

        case default_right:
            return "Right";
    }
    throw_exception("Don't know how to convert DisplayOrientation to string.");
}

Layout::Layout(circuit_id_t circuit_id) : circuit_id_ {circuit_id} {
    if (circuit_id < null_circuit) [[unlikely]] {
        throw_exception("Schematic id of layout cannot be negative.");
    }
}

auto Layout::swap(Layout &other) noexcept -> void {
    using std::swap;

    line_trees_.swap(other.line_trees_);
    positions_.swap(other.positions_);
    orientation_.swap(other.orientation_);
    display_states_.swap(other.display_states_);
    colors_.swap(other.colors_);

    swap(circuit_id_, other.circuit_id_);
}

auto swap(Layout &a, Layout &b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::Layout &a, logicsim::Layout &b) noexcept -> void {
    a.swap(b);
}

namespace logicsim {

auto Layout::format() const -> std::string {
    return "Layout( ... )";
}

auto Layout::add_default_element() -> element_id_t {
    line_trees_.push_back(LineTree {});
    positions_.push_back(point_t {});
    orientation_.push_back(DisplayOrientation::default_right);
    display_states_.push_back(DisplayState::normal);
    colors_.push_back(defaults::color_black);

    return element_id_t {
        gsl::narrow_cast<element_id_t::value_type>(positions_.size() - std::size_t {1})};
}

auto Layout::add_wire(LineTree &&line_tree) -> element_id_t {
    const auto id = add_default_element();

    line_trees_.back() = std::move(line_tree);

    return id;
}

auto Layout::add_logic_element(point_t position, DisplayOrientation orientation,
                               DisplayState display_state, color_t color)
    -> element_id_t {
    const auto id = add_default_element();

    positions_.back() = position;
    orientation_.back() = orientation;
    display_states_.back() = display_state;
    colors_.back() = color;

    return id;
}

auto Layout::set_line_tree(element_id_t element_id, LineTree &&line_tree) -> void {
    line_trees_.at(element_id.value) = std::move(line_tree);
}

auto Layout::set_position(element_id_t element_id, point_t position) -> void {
    positions_.at(element_id.value) = position;
}

auto Layout::circuit_id() const noexcept -> circuit_id_t {
    return circuit_id_;
}

auto Layout::line_tree(element_id_t element_id) const -> const LineTree & {
    return line_trees_.at(element_id.value);
}

auto Layout::position(element_id_t element_id) const -> point_t {
    return positions_.at(element_id.value);
}

auto Layout::orientation(element_id_t element_id) const -> DisplayOrientation {
    return orientation_.at(element_id.value);
}

auto Layout::display_state(element_id_t element_id) const -> DisplayState {
    return display_states_.at(element_id.value);
}

auto Layout::color(element_id_t element_id) const -> color_t {
    return colors_.at(element_id.value);
}

}  // namespace logicsim
