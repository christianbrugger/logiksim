#ifndef LOGIKSIM_SIMULATION_VIEW_H
#define LOGIKSIM_SIMULATION_VIEW_H

#include "algorithm/range.h"
#include "iterator_adaptor/transform_view.h"
#include "vocabulary.h"
#include "vocabulary/optional_logic_value.h"
#include "vocabulary/optional_logic_values.h"

#include <gsl/gsl>

namespace logicsim {

class Simulation;
class InteractiveSimulation;

namespace simulation {
class HistoryView;
}

namespace simulation_view {
class ConstElement;
}

class SimulationView {
    friend class simulation_view::ConstElement;

   public:
    [[nodiscard]] explicit SimulationView(const InteractiveSimulation &simulation);
    [[nodiscard]] explicit SimulationView(const Simulation &simulation,
                                          delay_t wire_delay_per_distance);

    // TODO rename to size
    [[nodiscard]] auto element_count() const noexcept -> std::size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    // TODO why do we need this?
    [[nodiscard]] auto is_element_id_valid(element_id_t element_id) const noexcept
        -> bool;

    // TODO re-use element_ids(Schematic ...)
    [[nodiscard]] auto element_ids() const noexcept -> forward_range_t<element_id_t>;
    [[nodiscard]] auto element(element_id_t element_id) const
        -> simulation_view::ConstElement;
    [[nodiscard]] inline auto elements() const;

    [[nodiscard]] auto time() const -> time_t;
    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;

   private:
    gsl::not_null<const Simulation *> simulation_;
    delay_t wire_delay_per_distance_;
};

namespace simulation_view {

class ConstElement {
    friend SimulationView;
    explicit ConstElement(const SimulationView &view, element_id_t element_id) noexcept;

   public:
    [[nodiscard]] auto has_connected_input(connection_id_t input_id) const -> bool;
    [[nodiscard]] auto has_connected_output(connection_id_t output_id) const -> bool;

    [[nodiscard]] auto input_value(connection_id_t index) const -> bool;
    [[nodiscard]] auto input_values() const -> const logic_small_vector_t &;
    [[nodiscard]] auto output_value(connection_id_t index) const -> OptionalLogicValue;
    [[nodiscard]] auto output_values() const -> optional_logic_values_t;

    [[nodiscard]] auto internal_state() const -> const logic_small_vector_t &;
    [[nodiscard]] auto internal_state(std::size_t index) const -> bool;

    // TODO add inverted

    [[nodiscard]] auto history_length() const -> delay_t;
    [[nodiscard]] auto input_history() const -> simulation::HistoryView;

    [[nodiscard]] auto time() const -> time_t;
    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;

   private:
    gsl::not_null<const SimulationView *> view_;
    element_id_t element_id_;
};

}  // namespace simulation_view

inline auto SimulationView::elements() const {
    return transform_view(element_ids(), [&](element_id_t element_id) {
        return this->element(element_id);
    });
}

}  // namespace logicsim

#endif
