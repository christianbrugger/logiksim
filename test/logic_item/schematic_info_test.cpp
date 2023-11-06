
#include "logic_item/schematic_info.h"

#include "algorithm/to_underlying.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace logicsim {

TEST(LogicItemSchematicInfo, LogicItemTypeConversion) {
    // assures that the conversion is transparent
    for (auto logicitem_type : all_logicitem_types) {
        const auto element_type = to_element_type(logicitem_type);
        EXPECT_EQ(to_underlying(logicitem_type), to_underlying(element_type));
    }

    // also test all values are correct
    for (auto element_type : all_element_types) {
        if (is_logic_item(element_type)) {
            const auto logicitem_type = to_logicitem_type(element_type);
            EXPECT_EQ(to_underlying(logicitem_type), to_underlying(element_type));
        } else {
            EXPECT_THROW(static_cast<void>(to_logicitem_type(element_type)),
                         std::exception);
        }
    }
}

TEST(LogicItemSchematicInfo, IsInputOutputCountValid) {
    //make sure no terminate is triggered
    for (auto element_type : all_element_types) {
        static_cast<void>(is_input_output_count_valid(
            element_type, connection_count_t {1}, connection_count_t {0}));
    }
}

}  // namespace logicsim
