#include "simulation_view.h"

#include "component/simulation/history_view.h"
#include "interactive_simulation.h"
#include "simulation.h"

namespace logicsim {

//
// Simulation View
//

SimulationView::SimulationView(const InteractiveSimulation& simulation)
    : simulation_ {&simulation.simulation()},
      wire_delay_per_distance_ {simulation.wire_delay_per_distance()} {}

SimulationView::SimulationView(const Simulation& simulation,
                               delay_t wire_delay_per_distance)
    : simulation_ {&simulation}, wire_delay_per_distance_ {wire_delay_per_distance} {}

auto SimulationView::element_count() const noexcept -> std::size_t {
    return simulation_->schematic().size();
}

auto SimulationView::empty() const noexcept -> bool {
    return simulation_->schematic().empty();
}

auto SimulationView::is_element_id_valid(element_id_t element_id) const noexcept -> bool {
    return element_id_t {0} <= element_id &&
           static_cast<std::size_t>(element_id.value) < element_count();
}

auto SimulationView::element_ids() const noexcept -> forward_range_t<element_id_t> {
    return forward_range_t<element_id_t> {element_id_t {element_count()}};
}

auto SimulationView::element(element_id_t element_id) const
    -> simulation_view::ConstElement {
    return simulation_view::ConstElement(*this, element_id);
}

auto SimulationView::time() const -> time_t {
    return simulation_->time();
}

auto SimulationView::wire_delay_per_distance() const -> delay_t {
    return wire_delay_per_distance_;
}

//
// Const Element
//

namespace simulation_view {

ConstElement::ConstElement(const SimulationView& view, element_id_t element_id) noexcept
    : view_ {&view}, element_id_ {element_id} {}

auto ConstElement::has_connected_input(connection_id_t input_id) const -> bool {
    const auto& schematic = view_->simulation_->schematic();
    return bool {schematic.output(input_t {element_id_, input_id})};
}

auto ConstElement::has_connected_output(connection_id_t output_id) const -> bool {
    const auto& schematic = view_->simulation_->schematic();

    const auto input = schematic.input(output_t {element_id_, output_id});
    return input &&
           !(schematic.element_type(input.element_id) == ElementType::placeholder);
}

auto ConstElement::input_value(connection_id_t index) const -> bool {
    return view_->simulation_->input_value(input_t {element_id_, index});
}

auto ConstElement::input_values() const -> const logic_small_vector_t& {
    return view_->simulation_->input_values(element_id_);
}

auto ConstElement::output_value(connection_id_t index) const -> bool {
    return view_->simulation_->output_value(output_t {element_id_, index});
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
    const auto& schematic = view_->simulation_->schematic();

    return schematic.history_length(element_id_);
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
