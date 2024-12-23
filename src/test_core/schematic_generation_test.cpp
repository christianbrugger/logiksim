
#include "core/schematic_generation.h"

#include "core/algorithm/sort_pair.h"
#include "core/layout.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(SchematicGeneration, EmptyLayout) {
    const auto layout = Layout {};
    const auto wire_delay_per_distance = delay_t {5us};

    const auto result = generate_schematic(layout, wire_delay_per_distance);

    ASSERT_EQ(result.schematic.size(), 0);
    ASSERT_EQ(result.line_trees.size(), 0);
    ASSERT_EQ(result.wire_delay_per_distance, wire_delay_per_distance);

    ASSERT_EQ(result.schematic, Schematic {});
}

TEST(SchematicGeneration, WireNoInputNoOutputConnected) {
    auto layout = Layout {};

    const auto wire_id = layout.wires().add_wire();
    auto& m_tree = layout.wires().modifiable_segment_tree(wire_id);

    m_tree.add_segment(segment_info_t {
        .line = ordered_line_t {point_t {0, 0}, point_t {1, 0}},
        .p0_type = SegmentPointType::output,
        .p1_type = SegmentPointType::output,
    });

    const auto schematic = generate_schematic(layout, delay_t {0us}).schematic;

    const auto wire_element = to_element_id(layout, wire_id);

    ASSERT_EQ(schematic.input_count(wire_element), connection_count_t {0});
    ASSERT_EQ(schematic.output_count(wire_element), connection_count_t {2});

    // order is implementation defined
    ASSERT_EQ(schematic.input(output_t {wire_element, connection_id_t {0}}), null_input);
    ASSERT_EQ(schematic.input(output_t {wire_element, connection_id_t {1}}), null_input);
}

TEST(SchematicGeneration, WireNoInputOneOutputConnected) {
    auto layout = Layout {};
    const auto logicitem_id = layout.logicitems().add(
        LogicItemDefinition {
            .logicitem_type = LogicItemType::and_element,
            .input_count = connection_count_t {2},
            .output_count = connection_count_t {1},
            .orientation = orientation_t::right,
        },
        point_t {1, 0}, display_state_t::normal);

    const auto wire_id = layout.wires().add_wire();
    auto& m_tree = layout.wires().modifiable_segment_tree(wire_id);

    m_tree.add_segment(segment_info_t {
        .line = ordered_line_t {point_t {0, 0}, point_t {1, 0}},
        .p0_type = SegmentPointType::output,
        .p1_type = SegmentPointType::output,
    });

    const auto schematic = generate_schematic(layout, delay_t {0us}).schematic;

    const auto and_element = to_element_id(layout, logicitem_id);
    const auto wire_element = to_element_id(layout, wire_id);

    ASSERT_EQ(schematic.input_count(wire_element), connection_count_t {0});
    ASSERT_EQ(schematic.output_count(wire_element), connection_count_t {2});

    // order is implementation defined
    const auto [con1, con2] =
        sorted(schematic.input(output_t {wire_element, connection_id_t {0}}),
               schematic.input(output_t {wire_element, connection_id_t {1}}));

    ASSERT_EQ(con1, null_input);
    ASSERT_EQ(con2, input_t(and_element, connection_id_t {0}));
}

TEST(SchematicGeneration, WireNoInputAllOutputsConnected) {
    auto layout = Layout {};
    const auto logicitem_id = layout.logicitems().add(
        LogicItemDefinition {
            .logicitem_type = LogicItemType::and_element,
            .input_count = connection_count_t {2},
            .output_count = connection_count_t {1},
            .orientation = orientation_t::right,
        },
        point_t {1, 0}, display_state_t::normal);

    const auto wire_id = layout.wires().add_wire();
    auto& m_tree = layout.wires().modifiable_segment_tree(wire_id);

    m_tree.add_segment(segment_info_t {
        .line = ordered_line_t {point_t {0, 0}, point_t {1, 0}},
        .p0_type = SegmentPointType::corner_point,
        .p1_type = SegmentPointType::output,
    });
    m_tree.add_segment(segment_info_t {
        .line = ordered_line_t {point_t {0, 0}, point_t {0, 1}},
        .p0_type = SegmentPointType::shadow_point,
        .p1_type = SegmentPointType::shadow_point,
    });
    m_tree.add_segment(segment_info_t {
        .line = ordered_line_t {point_t {0, 1}, point_t {1, 1}},
        .p0_type = SegmentPointType::corner_point,
        .p1_type = SegmentPointType::output,
    });

    const auto schematic = generate_schematic(layout, delay_t {0us}).schematic;

    const auto and_element = to_element_id(layout, logicitem_id);
    const auto wire_element = to_element_id(layout, wire_id);

    ASSERT_EQ(schematic.input_count(wire_element), connection_count_t {0});
    ASSERT_EQ(schematic.output_count(wire_element), connection_count_t {2});

    // order is implementation defined
    const auto [con1, con2] =
        sorted(schematic.input(output_t {wire_element, connection_id_t {0}}),
               schematic.input(output_t {wire_element, connection_id_t {1}}));

    ASSERT_EQ(con1, input_t(and_element, connection_id_t {0}));
    ASSERT_EQ(con2, input_t(and_element, connection_id_t {1}));
}

}  // namespace logicsim
