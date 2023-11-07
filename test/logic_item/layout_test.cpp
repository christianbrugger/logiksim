
#include "logic_item/layout.h"

#include "algorithm/range.h"
#include "layout_info.h"
#include "logging.h"
#include "random/generator.h"
#include "random/layout_calculation_data.h"
#include "vocabulary/logicitem_type.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cassert>
#include <vector>

namespace logicsim {

TEST(LogicItemLayout, InputCount) {
    for (auto logicitem_type : all_logicitem_types) {
        const auto info = get_layout_info(logicitem_type);

        EXPECT_TRUE(info.input_count_min <= info.input_count_max);

        EXPECT_TRUE(info.input_count_min <= info.input_count_default);
        EXPECT_TRUE(info.input_count_default <= info.input_count_max);

        if (info.static_inputs) {
            EXPECT_TRUE(info.static_inputs.value().size() <=
                        std::size_t {info.input_count_max});
        }
    }
}

TEST(LogicItemLayout, OutputCount) {
    for (auto logicitem_type : all_logicitem_types) {
        const auto info = get_layout_info(logicitem_type);

        EXPECT_TRUE(info.output_count_min <= info.output_count_max);

        EXPECT_TRUE(info.output_count_min <= info.output_count_default);
        EXPECT_TRUE(info.output_count_default <= info.output_count_max);

        if (info.static_outputs) {
            EXPECT_TRUE(info.static_outputs.value().size() <=
                        std::size_t {info.output_count_max});
        }
    }
}

TEST(LogicItemLayout, FixedOrVariableSize) {
    for (auto logicitem_type : all_logicitem_types) {
        const auto info = get_layout_info(logicitem_type);

        // never both set
        EXPECT_TRUE(info.fixed_width.has_value() + (info.variable_width != nullptr) <= 1);
        EXPECT_TRUE(info.fixed_height.has_value() + (info.variable_height != nullptr) <=
                    1);
    }
}

namespace {

template <typename R>
auto to_points(R inputs) -> std::vector<point_t> {
    auto result = std::vector<point_t> {};

    for (const auto &item : inputs) {
        result.push_back(item.position);
    }

    return result;
}

auto all_points_present(grid_t width, grid_t height, std::span<const point_t> body_points,
                        std::span<const point_t> inputs, std::span<const point_t> outputs)
    -> bool {
    auto expected = std::vector<point_t> {};
    for (auto x : range(width + grid_t {1})) {
        for (auto y : range(height + grid_t {1})) {
            expected.push_back(point_t {x, y});
        }
    }

    auto received = std::vector<point_t> {};
    std::ranges::copy(body_points, std::back_inserter(received));
    std::ranges::copy(inputs, std::back_inserter(received));
    std::ranges::copy(outputs, std::back_inserter(received));

    // sort & compare
    std::ranges::sort(expected);
    std::ranges::sort(received);
    const auto all_present = std::ranges::equal(expected, received);

    if (!all_present) {
        print();
        print("Error when comparing size & points:");
        print("expected:\n", expected);
        print("received (inputs + outputs + body_points):\n", received);
        print("width:", width, ", height:", height);
        print("inputs:", inputs);
        print("outputs:", outputs);
        print("body_points:", body_points);
        print();
    }

    return all_present;
}

}  // namespace

TEST(LogicItemLayout, StaticSizePositive) {
    for (auto logicitem_type : all_logicitem_types) {
        const auto info = get_layout_info(logicitem_type);

        if (info.fixed_width) {
            EXPECT_TRUE(info.fixed_width.value() >= grid_t {0});
        }
        if (info.fixed_height) {
            EXPECT_TRUE(info.fixed_width.value() >= grid_t {0});
        }
    }
}

TEST(LogicItemLayout, StaticBodyPoints) {
    for (auto logicitem_type : all_logicitem_types) {
        const auto &body_points = static_body_points_base(logicitem_type);

        if (!body_points) {
            continue;
        }

        const auto info = get_layout_info(logicitem_type);

        EXPECT_TRUE(all_points_present(info.fixed_width.value(),
                                       info.fixed_height.value(), body_points.value(),
                                       to_points(info.static_inputs.value()),
                                       to_points(info.static_outputs.value())));
    }
}

//
// Random tests
//

TEST(LogicItemLayout, RandomItems) {
    for (auto seed : range(1'000)) {
        auto rng = get_random_number_generator(seed);

        const auto data = get_random_layout_calculation_data(rng);
        const auto info = get_layout_info(data.logicitem_type);

        const auto inputs = input_locations_base(data);
        const auto outputs = output_locations_base(data);
        const auto body_points = element_body_points_base(data);

        const auto width = element_width(data);
        const auto height = element_height(data);

        // check counts
        EXPECT_TRUE(inputs.size() <= std::size_t {info.input_count_max});
        EXPECT_TRUE(outputs.size() <= std::size_t {info.output_count_max});

        // size positive
        EXPECT_TRUE(width >= grid_t {0});
        EXPECT_TRUE(height >= grid_t {0});

        // all points present
        EXPECT_TRUE(all_points_present(width, height, body_points, to_points(inputs),
                                       to_points(outputs)));
    }
}

}  // namespace logicsim