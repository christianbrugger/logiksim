#ifndef LOGIKSIM_SERIALIZE_DETAIL_H
#define LOGIKSIM_SERIALIZE_DETAIL_H

#include "vocabulary.h"

namespace logicsim::serialize {

constexpr static inline auto CURRENT_VERSION = 100;

struct SerializedLine {
    point_t p0;
    point_t p1;
};

struct SerializedLogicItem {
    ElementType element_type;
    std::size_t input_count;
    std::size_t output_count;

    logic_small_vector_t input_inverters;
    logic_small_vector_t output_inverters;

    point_t position;
    orientation_t orientation;
};

struct SerializedLayout {
    int version {CURRENT_VERSION};
    point_t save_position {0, 0};

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

auto save_file(std::string filename, std::string binary) -> void;

}  // namespace logicsim

#endif
