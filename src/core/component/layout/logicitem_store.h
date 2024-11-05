#ifndef LOGICSIM_COMPONENT_LAYOUT_LOGICITEM_STORE_H
#define LOGICSIM_COMPONENT_LAYOUT_LOGICITEM_STORE_H

#include "core/vocabulary/display_state.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/logicitem_type.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/rect.h"

#include <ankerl/unordered_dense.h>

#include <type_traits>
#include <vector>

namespace logicsim {

struct layout_calculation_data_t;

namespace layout {

template <typename T>
    requires std::is_nothrow_move_constructible_v<T>
using attr_map_t = ankerl::unordered_dense::map<logicitem_id_t, T>;

/**
 * @brief: Stores the logicitem data of the layout.
 *
 * Class invariants:
 *     + All stored logic-item form valid definitions.
 *     + All data vectors have the same size.
 *     + Input & output inverters vectors have size of input & output count.
 *     + All bounding rects are fully representable on the grid.
 */
class LogicItemStore {
   public:
    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    // add & delete
    auto add(LogicItemDefinition &&definition, point_t position,
             display_state_t display_state) -> logicitem_id_t;
    auto swap_and_delete(logicitem_id_t logicitem_id) -> logicitem_id_t;
    auto swap_items(logicitem_id_t logicitem_id_1, logicitem_id_t logicitem_id_2) -> void;

    /**
     * @brief: brings the store in canonical form,
     *         so that visual equivalent layouts compare equal
     */
    auto normalize() -> void;
    [[nodiscard]] auto operator==(const LogicItemStore &) const -> bool = default;

    // getters
    [[nodiscard]] auto type(logicitem_id_t logicitem_id) const -> LogicItemType;
    [[nodiscard]] auto input_count(logicitem_id_t logicitem_id) const
        -> connection_count_t;
    [[nodiscard]] auto output_count(logicitem_id_t logicitem_id) const
        -> connection_count_t;
    [[nodiscard]] auto orientation(logicitem_id_t logicitem_id) const -> orientation_t;
    [[nodiscard]] auto sub_circuit_id(logicitem_id_t logicitem_id) const -> circuit_id_t;
    [[nodiscard]] auto input_inverters(logicitem_id_t logicitem_id) const
        -> logic_small_vector_t;
    [[nodiscard]] auto output_inverters(logicitem_id_t logicitem_id) const
        -> logic_small_vector_t;
    [[nodiscard]] auto position(logicitem_id_t logicitem_id) const -> point_t;
    [[nodiscard]] auto display_state(logicitem_id_t logicitem_id) const
        -> display_state_t;
    [[nodiscard]] auto bounding_rect(logicitem_id_t logicitem_id) const -> rect_t;
    [[nodiscard]] auto attrs_clock_generator(logicitem_id_t logicitem_id) const
        -> const attributes_clock_generator_t &;

    [[nodiscard]] auto input_inverted(logicitem_id_t logicitem_id,
                                      connection_id_t input_id) const -> bool;
    [[nodiscard]] auto output_inverted(logicitem_id_t logicitem_id,
                                       connection_id_t output_id) const -> bool;

    // setters
    auto set_position(logicitem_id_t logicitem_id, point_t position) -> void;
    auto set_display_state(logicitem_id_t logicitem_id,
                           display_state_t display_state) -> void;
    auto set_attributes(logicitem_id_t logicitem_id,
                        attributes_clock_generator_t attrs) -> void;
    auto set_input_inverter(logicitem_id_t logicitem_id, connection_id_t connection_id,
                            bool value) -> void;
    auto set_output_inverter(logicitem_id_t logicitem_id, connection_id_t connection_id,
                             bool value) -> void;

   private:
    auto delete_last() -> void;
    auto last_logicitem_id() const -> logicitem_id_t;

   private:
    std::vector<LogicItemType> logicitem_types_ {};
    std::vector<connection_count_t> input_counts_ {};
    std::vector<connection_count_t> output_counts_ {};
    std::vector<orientation_t> orientations_ {};

    std::vector<circuit_id_t> sub_circuit_ids_ {};
    std::vector<logic_small_vector_t> input_inverters_ {};
    std::vector<logic_small_vector_t> output_inverters_ {};

    std::vector<point_t> positions_ {};
    std::vector<display_state_t> display_states_ {};
    std::vector<rect_t> bounding_rects_ {};

    layout::attr_map_t<attributes_clock_generator_t> map_clock_generator_ {};
};

//
// Free Functions
//

[[nodiscard]] auto to_layout_calculation_data(const LogicItemStore &store,
                                              logicitem_id_t logicitem_id)
    -> layout_calculation_data_t;

[[nodiscard]] auto to_layout_calculation_data(
    const LogicItemStore &store, logicitem_id_t logicitem_id,
    point_t position) -> layout_calculation_data_t;

[[nodiscard]] auto to_logicitem_definition(
    const LogicItemStore &store, logicitem_id_t logicitem_id) -> LogicItemDefinition;

}  // namespace layout

}  // namespace logicsim

#endif
