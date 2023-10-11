#ifndef LOGICSIM_VOCABULARY_CONNECTOR_INFO_H
#define LOGICSIM_VOCABULARY_CONNECTOR_INFO_H

#include "vocabulary/orientation.h"
#include "vocabulary/point.h"

namespace logicsim {

struct connector_info_t {
    point_t position;
    orientation_t orientation;
};

}  // namespace logicsim

#endif
