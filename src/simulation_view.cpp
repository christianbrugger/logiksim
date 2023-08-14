#include "simulation_view.h"

#include "simulation.h"

namespace logicsim {
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

namespace simulation_view {

ConstElement::ConstElement(const SimulationView& view, element_id_t element_id) noexcept
    : view_ {&view}, element_id_ {element_id} {}

}  // namespace simulation_view

//
}  // namespace logicsim
