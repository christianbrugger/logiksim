#ifndef LOGICSIM_VOCABULARY_CONNECTOR_INFO_H
#define LOGICSIM_VOCABULARY_CONNECTOR_INFO_H

#include "format/struct.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

#include <string>

namespace logicsim {

/**
 * @brief: Input connector position and orientation.
 */
struct simple_input_info_t {
    point_t position;
    orientation_t orientation;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<simple_input_info_t>);

/**
 * @brief: Output connector position and orientation.
 */
struct simple_output_info_t {
    point_t position;
    orientation_t orientation;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<simple_output_info_t>);

/**
 * @brief: Input id, position and orientation.
 */
struct extended_input_info_t {
    point_t position;
    connection_id_t input_id;
    orientation_t orientation;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<extended_input_info_t>);

/**
 * @brief: Converts simple input to extended info.
 *
 * Note we don't use a constructor, so  extended_connector_info stays an aggregate.
 */
[[nodiscard]] constexpr inline auto extend_input_info(connection_id_t input_id,
                                                      simple_input_info_t simple_info)
    -> extended_input_info_t;

/**
 * @brief: Output id, position and orientation.
 */
struct extended_output_info_t {
    point_t position;
    connection_id_t output_id;
    orientation_t orientation;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_aggregate_v<extended_output_info_t>);

/**
 * @brief: Converts simple output to extended info.
 *
 * Note we don't use a constructor, so  extended_connector_info stays an aggregate.
 */
[[nodiscard]] constexpr inline auto extend_output_info(connection_id_t output_id,
                                                       simple_output_info_t simple_info)
    -> extended_output_info_t;

//
// Implementation
//

constexpr inline auto extend_input_info(connection_id_t input_id,
                                        simple_input_info_t simple_info)
    -> extended_input_info_t {
    return extended_input_info_t {
        .position = simple_info.position,
        .input_id = input_id,
        .orientation = simple_info.orientation,
    };
}

constexpr inline auto extend_output_info(connection_id_t output_id,
                                         simple_output_info_t simple_info)
    -> extended_output_info_t {
    return extended_output_info_t {
        .position = simple_info.position,
        .output_id = output_id,
        .orientation = simple_info.orientation,
    };
}

}  // namespace logicsim

#endif
