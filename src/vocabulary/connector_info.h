#ifndef LOGICSIM_VOCABULARY_CONNECTOR_INFO_H
#define LOGICSIM_VOCABULARY_CONNECTOR_INFO_H

#include "format/struct.h"
#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

#include <string>

namespace logicsim {

/**
 * @brief: Connector position and orientation.
 */
struct connector_info_t {
    point_t position;
    orientation_t orientation;

    [[nodiscard]] auto format() -> std::string;
};

}  // namespace logicsim

#endif
