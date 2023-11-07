#ifndef LOGIKSIM_SERIALIZE_DETAIL_H
#define LOGIKSIM_SERIALIZE_DETAIL_H

#include "vocabulary/connection_count.h"
#include "vocabulary/logicitem_definition.h"
#include "vocabulary/logicitem_type.h"
#include "vocabulary/logic_small_vector.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"
#include "vocabulary/point_fine.h"

#include <cstdint>
#include <optional>
#include <string>

namespace logicsim::serialize {

constexpr static inline auto CURRENT_VERSION = 100;

struct SerializedLine {
    point_t p0;
    point_t p1;
};

constexpr static inline auto name_max_size = 100;

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

struct SerializedViewConfig {
    double device_scale {};
    grid_fine_t grid_offset_x {};
    grid_fine_t grid_offset_y {};
};

struct SerializedSimulationSettings {
    int64_t simulation_time_rate_ns {10'000};
    bool use_wire_delay {true};
};

struct SerializedLayout {
    int version {CURRENT_VERSION};
    // used for copy & paste
    point_t save_position {0, 0};
    // view config
    SerializedViewConfig view_config {};
    SerializedSimulationSettings simulation_settings {};

    std::vector<SerializedLogicItem> logic_items {};
    std::vector<SerializedLine> wire_segments {};
};

}  // namespace logicsim::serialize

namespace logicsim {
auto json_dumps(const serialize::SerializedLayout& data) -> std::string;
auto json_loads(std::string value) -> std::optional<serialize::SerializedLayout>;

}  // namespace logicsim

#endif
