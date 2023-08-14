#include "simulation_view.h"

#include "simulation.h"

namespace logicsim {

//
// Simulation View
//

SimulationView::SimulationView(const Simulation& simulation)
    : schematic_ {&simulation.schematic()}, simulation_ {&simulation} {}

auto SimulationView::element_count() const noexcept -> std::size_t {
    return schematic_->element_count();
}

auto SimulationView::empty() const noexcept -> bool {
    return schematic_->empty();
}

auto SimulationView::is_element_id_valid(element_id_t element_id) const noexcept -> bool {
    return schematic_->is_element_id_valid(element_id);
}

auto SimulationView::element_ids() const noexcept -> forward_range_t<element_id_t> {
    return schematic_->element_ids();
}

auto SimulationView::element(element_id_t element_id) const
    -> simulation_view::ConstElement {
    return simulation_view::ConstElement(*this, element_id);
}

auto SimulationView::time() const -> time_t {
    return simulation_->time();
}

auto SimulationView::wire_delay_per_distance() const -> delay_t {
    return schematic_->wire_delay_per_distance();
}

//
// Const Element
//

namespace simulation_view {

ConstElement::ConstElement(const SimulationView& view, element_id_t element_id) noexcept
    : view_ {&view}, element_id_ {element_id} {}

auto ConstElement::has_connected_input(connection_id_t index) const -> bool {
    return view_->schematic_->element(element_id_).input(index).has_connected_element();
}

auto ConstElement::has_connected_output(connection_id_t index) const -> bool {
    const auto output = view_->schematic_->element(element_id_).output(index);
    return output.has_connected_element() && !output.connected_element().is_placeholder();
}

auto ConstElement::input_value(connection_id_t index) const -> bool {
    return view_->simulation_->input_value(element_id_, index);
}

auto ConstElement::input_values() const -> const logic_small_vector_t& {
    return view_->simulation_->input_values(element_id_);
}

auto ConstElement::output_value(connection_id_t index) const -> bool {
    return view_->simulation_->output_value(element_id_, index);
}

auto ConstElement::output_values() const -> logic_small_vector_t {
    return view_->simulation_->output_values(element_id_);
}

auto ConstElement::internal_state() const -> const logic_small_vector_t& {
    return view_->simulation_->internal_state(element_id_);
}

auto ConstElement::internal_state(std::size_t index) const -> bool {
    return internal_state().at(index);
}

auto ConstElement::history_length() const -> delay_t {
    return view_->schematic_->element(element_id_).history_length();
}

auto ConstElement::input_history() const -> simulation::HistoryView {
    return view_->simulation_->input_history(element_id_);
}

auto ConstElement::time() const -> time_t {
    return view_->time();
}

auto ConstElement::wire_delay_per_distance() const -> delay_t {
    return view_->wire_delay_per_distance();
}

}  // namespace simulation_view
}  // namespace logicsim
