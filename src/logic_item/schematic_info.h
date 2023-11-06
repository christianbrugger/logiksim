#ifndef LOGICSIM_LOGIC_ITEM_SCHEMATIC_INFO_H
#define LOGICSIM_LOGIC_ITEM_SCHEMATIC_INFO_H

#include "vocabulary/delay.h"
#include "vocabulary/element_type.h"
#include "vocabulary/internal_connections.h"
#include "vocabulary/logicitem_type.h"

namespace logicsim {

[[nodiscard]] auto to_element_type(LogicItemType logicitem_type) -> ElementType;
[[nodiscard]] auto to_logicitem_type(ElementType logicitem_type) -> LogicItemType;


[[nodiscard]] auto element_enable_input_id(ElementType element_type)
    -> std::optional<connection_id_t>;

[[nodiscard]] auto element_output_delay(LogicItemType logicitem_type) -> delay_t;

[[nodiscard]] auto element_internal_connections(ElementType element_type)
    -> internal_connections_t;

[[nodiscard]] auto has_internal_connections(ElementType element_type) -> bool;

[[nodiscard]] auto is_input_output_count_valid(ElementType element_type,
                                               connection_count_t input_count,
                                               connection_count_t output_count) -> bool;

}  // namespace logicsim

#endif
