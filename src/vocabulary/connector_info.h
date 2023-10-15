#ifndef LOGICSIM_VOCABULARY_CONNECTOR_INFO_H
#define LOGICSIM_VOCABULARY_CONNECTOR_INFO_H

#include "format/struct.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

#include <string>

namespace logicsim {

/**
 * @brief: Connector position and orientation.
 */
struct simple_connector_info_t {
    point_t position;
    orientation_t orientation;

    [[nodiscard]] auto format() -> std::string;
};

static_assert(std::is_aggregate_v<simple_connector_info_t>);

/**
 * @brief: Connector id, position and orientation.
 */
struct extended_connector_info_t {
    point_t position;
    connection_id_t id;
    orientation_t orientation;

    [[nodiscard]] auto format() -> std::string;
};

static_assert(std::is_aggregate_v<extended_connector_info_t>);

/**
 * @brief: Converts simple to extended info.
 *
 * Note we don't use a constructor, so  extended_connector_info stays an aggregate.
 */
[[nodiscard]] constexpr inline auto extend_connector_info(
    connection_id_t id, simple_connector_info_t simple_info)
    -> extended_connector_info_t {
    return extended_connector_info_t {
        .position = simple_info.position,
        .id = id,
        .orientation = simple_info.orientation,
    };
}

}  // namespace logicsim

#endif
