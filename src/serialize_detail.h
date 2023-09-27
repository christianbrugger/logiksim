#ifndef LOGIKSIM_SERIALIZE_DETAIL_H
#define LOGIKSIM_SERIALIZE_DETAIL_H

#include "vocabulary.h"

namespace logicsim::serialize {

constexpr static inline auto CURRENT_VERSION = 100;

struct SerializedLine {
    point_t p0;
    point_t p1;
};

constexpr static inline auto name_max_size = 100;

struct SerializedAttributesClockGenerator {
    std::string name;

    int32_t time_symmetric_ns;
    int32_t time_on_ns;
    int32_t time_off_ns;

    bool is_symmetric;
    bool show_simulation_controls;
};

struct SerializedLogicItem {
    ElementType element_type;
    connection_count_t input_count;
    connection_count_t output_count;

    logic_small_vector_t input_inverters;
    logic_small_vector_t output_inverters;

    point_t position;
    orientation_t orientation;

    std::optional<SerializedAttributesClockGenerator> attributes_clock_generator;
};

struct SerializedViewConfig {
    double device_scale {};
    double grid_offset_x {};
    double grid_offset_y {};
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

auto gzip_compress(const std::string& input) -> std::string;
auto gzip_decompress(const std::string& input) -> std::string;

auto base64_encode(const std::string& data) -> std::string;
auto base64_decode(const std::string& data) -> std::string;

}  // namespace logicsim

#endif
