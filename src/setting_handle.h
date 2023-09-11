#ifndef LOGIKSIM_SETTING_HANDLE
#define LOGIKSIM_SETTING_HANDLE

#include "resource.h"
#include "vocabulary.h"

#include <blend2d.h>

#include <optional>

namespace logicsim {

namespace defaults {
constexpr static inline auto setting_handle_size = grid_fine_t {1.0};
constexpr static inline auto setting_handle_margin = grid_fine_t {0.1};
}  // namespace defaults

class Layout;
class Selection;

class ViewConfig;

struct setting_handle_t {
    point_fine_t position;
    icon_t icon;
};

auto setting_handle_position(const Layout& layout, element_id_t element_id)
    -> std::optional<setting_handle_t>;

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t>;

auto setting_handle_rect(setting_handle_t handle, const ViewConfig& config)
    -> rect_fine_t;

auto get_colliding_setting_handle(point_fine_t position, const Layout& layout,
                                  const Selection& selection, const ViewConfig& config)
    -> std::optional<setting_handle_t>;

}  // namespace logicsim

#endif