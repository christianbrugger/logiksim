#ifndef LOGIKSIM_SETTING_HANDLE
#define LOGIKSIM_SETTING_HANDLE

#include "vocabulary.h"

#include <blend2d.h>

#include <optional>

namespace logicsim {

class Layout;
class Selection;

class ViewConfig;

struct setting_handle_t {
    point_fine_t position;
};

auto setting_handle_position(const Layout& layout, element_id_t element_id)
    -> setting_handle_t;

auto setting_handle_positions(const Layout& layout, const Selection& selection)
    -> setting_handle_t;

auto size_handle_rect_px(setting_handle_t handle, const ViewConfig& config) -> BLRect;

auto size_handle_rect_gird(setting_handle_t handle, const ViewConfig& config)
    -> rect_fine_t;

auto get_colliding_settings_handle(point_fine_t position, const Layout& layout,
                                   const Selection& selection, const ViewConfig& config)
    -> std::optional<setting_handle_t>;

}  // namespace logicsim

#endif