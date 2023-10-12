#ifndef LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_H
#define LOGICSIM_LOGIC_ITEM_LAYOUT_DISPLAY_H

#include "vocabulary/connection_id.h"
#include "vocabulary/font_style.h"
#include "vocabulary/grid_fine.h"

namespace logicsim {

/**
 * @brief: Commont between all displays.
 */
namespace display {

constexpr static inline auto font_style = FontStyle::monospace;
constexpr static inline auto font_size = grid_fine_t {0.9};
constexpr static inline auto enable_input_id = connection_id_t {0};

constexpr static inline auto margin_horizontal = grid_fine_t {0.2};
constexpr static inline auto padding_vertical = grid_fine_t {0.7};
constexpr static inline auto padding_horizontal = grid_fine_t {0.25};

}  // namespace display

}  // namespace logicsim

#endif
