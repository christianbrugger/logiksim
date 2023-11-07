
#include "layout.h"

#include "vocabulary/element_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

namespace logicsim {

TEST(Layout, EmptyLayout) {
    const auto layout = Layout {};

    EXPECT_EQ(layout.size(), 0);
    EXPECT_EQ(layout.empty(), true);
    EXPECT_EQ(std::ranges::distance(logicitem_ids(layout)), 0);
    EXPECT_EQ(std::ranges::distance(wire_ids(layout)), 0);
}

TEST(Layout, LayoutSingleLogicItem) {
    auto layout = Layout {};

    layout.logic_items().add(
        ElementDefinition {
            .logicitem_type = LogicItemType::buffer_element,
            .input_count = connection_count_t {1},
            .output_count = connection_count_t {1},
            .orientation = orientation_t::right,
        },
        point_t {}, display_state_t::temporary);

    EXPECT_EQ(layout.size(), 1);
    EXPECT_EQ(std::ranges::distance(logicitem_ids(layout)), 1);
    EXPECT_EQ(std::ranges::distance(wire_ids(layout)), 0);
}

TEST(Layout, LayoutSingleWire) {
    auto layout = Layout {};

    layout.wires().add_wire();

    EXPECT_EQ(layout.size(), 3);
    EXPECT_EQ(std::ranges::distance(logicitem_ids(layout)), 0);
    EXPECT_EQ(std::ranges::distance(wire_ids(layout)), 3);
}

TEST(Layout, ElementProperties) {
    auto layout = Layout {};

    const auto input_inverters = logic_small_vector_t {false, true, false};
    const auto output_inverters = logic_small_vector_t {true};

    layout.logic_items().add(
        ElementDefinition {
            .logicitem_type = LogicItemType::and_element,
            .input_count = connection_count_t {3},
            .output_count = connection_count_t {1},
            .orientation = orientation_t::right,

            .sub_circuit_id = circuit_id_t {10},
            .input_inverters = input_inverters,
            .output_inverters = output_inverters,
        },
        point_t {2, 3}, display_state_t::colliding);

    const Layout& layout_const {layout};
    const auto id = logicitem_id_t {0};

    EXPECT_EQ(layout_const.logic_items().type(id), LogicItemType::and_element);
    EXPECT_EQ(layout_const.logic_items().input_count(id), connection_count_t {3});
    EXPECT_EQ(layout_const.logic_items().output_count(id), connection_count_t {1});
    EXPECT_EQ(layout_const.logic_items().orientation(id), orientation_t::right);

    EXPECT_EQ(layout_const.logic_items().sub_circuit_id(id), circuit_id_t {10});
    EXPECT_EQ(layout_const.logic_items().input_inverters(id), input_inverters);
    EXPECT_EQ(layout_const.logic_items().output_inverters(id), output_inverters);

    EXPECT_EQ(layout_const.logic_items().position(id), point_t(2, 3));
    EXPECT_EQ(layout_const.logic_items().display_state(id), display_state_t::colliding);
}

TEST(Layout, EqualityOperators) {
    auto layout = Layout {};

    const auto definition = ElementDefinition {
        .logicitem_type = LogicItemType::buffer_element,
        .input_count = connection_count_t {1},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };

    auto element_0 =
        layout.logic_items().add(definition, point_t {}, display_state_t::temporary);
    auto element_1 =
        layout.logic_items().add(definition, point_t {}, display_state_t::temporary);

    EXPECT_NE(element_0, element_1);
    EXPECT_EQ(element_0, logicitem_id_t {0});
    EXPECT_EQ(element_1, logicitem_id_t {1});

    EXPECT_EQ(layout.size(), 2);
}

}  // namespace logicsim