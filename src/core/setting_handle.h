#ifndef LOGIKSIM_SETTING_HANDLE
#define LOGIKSIM_SETTING_HANDLE

#include "core/resource.h"
#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/point_fine.h"

#include <optional>

namespace logicsim {

class Layout;
class Selection;
struct ViewConfig;
struct logicitem_id_t;
struct rect_fine_t;

namespace defaults {
constexpr static inline auto setting_handle_size = grid_fine_t {1.0};
constexpr static inline auto setting_handle_margin = grid_fine_t {0.1};
}  // namespace defaults

struct setting_handle_t {
    point_fine_t position;
    icon_t icon;
    logicitem_id_t logicitem_id;
};

auto setting_handle_position(const Layout& layout, logicitem_id_t logicitem_id)
    -> std::optional<setting_handle_t>;

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t>;

auto setting_handle_rect(setting_handle_t handle) -> rect_fine_t;

auto is_colliding(setting_handle_t handle, point_fine_t position) -> bool;

auto get_colliding_setting_handle(point_fine_t position, const Layout& layout,
                                  const Selection& selection)
    -> std::optional<setting_handle_t>;

}  // namespace logicsim

#endif