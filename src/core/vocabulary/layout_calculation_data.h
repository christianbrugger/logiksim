#ifndef LOGICSIM_VOCABULARY_LAYOUT_CALCULATION_DATA_H
#define LOGICSIM_VOCABULARY_LAYOUT_CALCULATION_DATA_H

#include "format/struct.h"
#include "vocabulary/connection_count.h"
#include "vocabulary/logicitem_type.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

#include <compare>
#include <cstddef>
#include <string>
#include <type_traits>

namespace logicsim {

struct LogicItemDefinition;
struct PlacedLogicItem;

// TODO rename to logicitem_layout_data_t
/**
 * @brief: Logic item data required to calculate its layout.
 */
struct layout_calculation_data_t {
    std::size_t internal_state_count {0};
    point_t position {0, 0};
    connection_count_t input_count {0};
    connection_count_t output_count {0};
    orientation_t orientation {orientation_t::undirected};
    LogicItemType logicitem_type {LogicItemType::sub_circuit};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const layout_calculation_data_t& other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const layout_calculation_data_t& other) const =
        default;
};

static_assert(std::is_aggregate_v<layout_calculation_data_t>);

[[nodiscard]] auto to_layout_calculation_data(
    const LogicItemDefinition& definition, point_t position) -> layout_calculation_data_t;

[[nodiscard]] auto to_layout_calculation_data(const PlacedLogicItem& element)
    -> layout_calculation_data_t;

}  // namespace logicsim

#endif
