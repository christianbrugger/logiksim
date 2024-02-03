#ifndef LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H
#define LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H

#include "layout.h"
#include "vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

inline auto add_and_element(Layout &layout, display_state_t display_type,
                            connection_count_t input_count = connection_count_t {3},
                            point_t position = point_t {0, 0}) -> logicitem_id_t {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::and_element,

        .input_count = input_count,
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    return layout.logic_items().add(definition, position, display_type);
}

inline auto assert_logicitem_count(const Layout &layout, std::size_t count) -> void {
    ASSERT_EQ(layout.logic_items().size(), count);
}

inline auto assert_logicitem_equal(
    const Layout &layout, logicitem_id_t logicitem_id,
    connection_count_t input_count = connection_count_t {3},
    point_t position = point_t {0, 0}) -> void {
    ASSERT_EQ(layout.logic_items().input_count(logicitem_id), input_count);
    ASSERT_EQ(layout.logic_items().position(logicitem_id), position);
}

}  // namespace logicsim

#endif