
#include "layout.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>

namespace logicsim {

TEST(Layout, EmptyLayout) {
    const Layout layout;

    EXPECT_EQ(layout.element_count(), 0);
    EXPECT_EQ(layout.empty(), true);
    EXPECT_EQ(std::ranges::distance(layout.element_ids()), 0);
    EXPECT_EQ(std::ranges::distance(layout.elements()), 0);

    layout.validate();
}

TEST(Layout, LayoutSingleElement) {
    Layout layout;

    layout.add_element(Layout::ElementData {});

    EXPECT_EQ(layout.element_count(), 1);
    EXPECT_EQ(std::ranges::distance(layout.element_ids()), 1);
    EXPECT_EQ(std::ranges::distance(layout.elements()), 1);

    layout.validate();
}

TEST(Layout, ElementProperties) {
    Layout layout;

    layout.add_element(Layout::ElementData {
        .element_type = ElementType::and_element,
        .display_state = display_state_t::normal,

        .input_count = 3,
        .output_count = 1,
        .position = point_t {2, 3},
        .orientation = orientation_t::right,
        .color = defaults::color_green,

        .circuit_id = circuit_id_t {10},
    });

    const Layout& layout_const {layout};
    const layout::ConstElement element {layout_const.element(element_id_t {0})};

    EXPECT_EQ(element.element_id(), element_id_t {0});
    EXPECT_EQ(element.element_type(), ElementType::and_element);

    EXPECT_EQ(element.input_count(), 3);
    EXPECT_EQ(element.output_count(), 1);
    EXPECT_EQ(element.position(), point_t(2, 3));
    EXPECT_EQ(element.orientation(), orientation_t::right);
    EXPECT_EQ(element.color(), defaults::color_green);

    layout.validate();
    layout_const.validate();
}

TEST(Layout, EqualityOperators) {
    Layout layout;
    const Layout& layout_const {layout};

    auto element_0 = layout.add_element(Layout::ElementData {
        .element_type = ElementType::and_element,
        .input_count = 2,
        .output_count = 1,
    });
    auto element_1 = layout.add_element(Layout::ElementData {
        .element_type = ElementType::and_element,
        .input_count = 2,
        .output_count = 1,
    });

    EXPECT_NE(element_0, element_1);
    EXPECT_EQ(element_0, layout.element(element_id_t {0}));
    EXPECT_EQ(element_1, layout.element(element_id_t {1}));

    EXPECT_NE(element_0, layout_const.element(element_id_t {1}));
    EXPECT_EQ(element_0, layout_const.element(element_id_t {0}));
    EXPECT_EQ(element_1, layout_const.element(element_id_t {1}));
}

TEST(Layout, ElementConversions) {
    Layout layout;

    auto element = layout.add_element(Layout::ElementData {
        .element_type = ElementType::and_element,
        .input_count = 2,
        .output_count = 1,
    });

    const auto element_const = layout::ConstElement {element};
    const auto element_id = element_id_t {element};

    EXPECT_EQ(element, element_const);
    EXPECT_EQ(element_id, element_id_t {0});
}

//
// Element View
//

TEST(Layout, ElementViewEmpty) {
    Layout layout;
    auto view = layout.elements();

    ASSERT_THAT(view, testing::ElementsAre());
    ASSERT_EQ(std::ranges::distance(view), 0);

    const Layout& layout_const {layout};
    auto view_const = layout_const.elements();

    ASSERT_THAT(view_const, testing::ElementsAre());
    ASSERT_EQ(std::ranges::distance(view_const), 0);
}

TEST(Layout, ElementViewSome) {
    Layout layout;

    auto and_element = layout.add_element(Layout::ElementData {
        .element_type = ElementType::and_element,
        .input_count = 2,
        .output_count = 1,
    });
    auto inverter = layout.add_element(Layout::ElementData {
        .element_type = ElementType::inverter_element,
        .input_count = 1,
        .output_count = 1,
    });

    auto view = layout.elements();

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);
    ASSERT_EQ(std::ranges::distance(std::ranges::begin(view), std::ranges::end(view)), 2);
    ASSERT_EQ(std::ranges::distance(view), 2);

    ASSERT_THAT(view, testing::ElementsAre(and_element, inverter));
}

}  // namespace logicsim