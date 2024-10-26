#ifndef LOGIKSIM_SERIALIZE_DETAIL_H
#define LOGIKSIM_SERIALIZE_DETAIL_H

#include "core/vocabulary/connection_count.h"
#include "core/vocabulary/decoration_type.h"
#include "core/vocabulary/font_style.h"
#include "core/vocabulary/load_error.h"
#include "core/vocabulary/logic_small_vector.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/logicitem_type.h"
#include "core/vocabulary/orientation.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/point_fine.h"
#include "core/vocabulary/size_2d.h"
#include "core/vocabulary/text_alignment.h"

#include <tl/expected.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace logicsim::serialize {

/**
 * @brief: Save file version, always increasing.
 *
 * 100: LogikSim 2.1.0
 * 200: LogikSim 2.2.0
 */
constexpr static inline auto CURRENT_VERSION = 200;

/**
 * @brief: Minimum LogikSim version for the save-file version
 *
 * Stored in the json, so older version know what to upgrade to.
 */
constexpr static inline auto MIN_LS_APP_VERSION_STR = "2.2.0";

struct SerializedLine {
    point_t p0;
    point_t p1;
};

constexpr static inline auto clock_generator_name_max_size = 100;
constexpr static inline auto text_element_text_max_size = 10'000;
constexpr static inline auto minimum_logiksim_version_max_size = 20;

struct SerializedAttributesClockGenerator {
    std::string name;

    int64_t time_symmetric_ns;
    int64_t time_on_ns;
    int64_t time_off_ns;

    bool is_symmetric;
    bool show_simulation_controls;
};

struct SerializedLogicItem {
    LogicItemType logicitem_type;
    connection_count_t::value_type_rep input_count;
    connection_count_t::value_type_rep output_count;

    logic_small_vector_t input_inverters;
    logic_small_vector_t output_inverters;

    point_t position;
    orientation_t orientation;

    std::optional<SerializedAttributesClockGenerator> attributes_clock_generator;
};

struct SerializedRgbColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct SerializedAttributesTextElement {
    std::string text;

    HTextAlignment horizontal_alignment;
    FontStyle font_style;
    SerializedRgbColor text_color;
};

struct SerializedDecoration {
    DecorationType decoration_type;
    point_t position;
    size_2d_t size;

    std::optional<SerializedAttributesTextElement> attributes_text_element;
};

struct SerializedViewPoint {
    double device_scale {};
    grid_fine_t grid_offset_x {};
    grid_fine_t grid_offset_y {};
};

struct SerializedSimulationConfig {
    int64_t simulation_time_rate_ns {10'000};
    bool use_wire_delay {true};
};

struct SerializedLayout {
    int version {CURRENT_VERSION};
    std::string minimum_logiksim_version {MIN_LS_APP_VERSION_STR};
    // used for copy & paste
    point_t save_position {0, 0};
    // used for file loading
    SerializedViewPoint view_point {};
    SerializedSimulationConfig simulation_config {};

    std::vector<SerializedLogicItem> logicitems {};
    std::vector<SerializedDecoration> decorations {};
    std::vector<SerializedLine> wire_segments {};
};

}  // namespace logicsim::serialize

namespace logicsim {
auto json_dumps(const serialize::SerializedLayout& data) -> std::string;
auto json_loads(std::string_view text)
    -> tl::expected<serialize::SerializedLayout, LoadError>;

}  // namespace logicsim

#endif
