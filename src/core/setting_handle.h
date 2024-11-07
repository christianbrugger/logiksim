#ifndef LOGIKSIM_SETTING_HANDLE
#define LOGIKSIM_SETTING_HANDLE

#include "core/format/struct.h"
#include "core/resource.h"
#include "core/vocabulary/decoration_id.h"
#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/logicitem_id.h"
#include "core/vocabulary/point_fine.h"

#include <optional>
#include <variant>

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
    std::variant<logicitem_id_t, decoration_id_t> element_id;

    [[nodiscard]] auto operator==(const setting_handle_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] auto setting_handle_position(
    const Layout& layout, logicitem_id_t logicitem_id) -> std::optional<setting_handle_t>;

[[nodiscard]] auto setting_handle_position(const Layout& layout,
                                           decoration_id_t decoration_id)
    -> std::optional<setting_handle_t>;

[[nodiscard]] auto setting_handle_position(
    const Layout& layout, const Selection& selection) -> std::optional<setting_handle_t>;

[[nodiscard]] auto setting_handle_rect(setting_handle_t handle) -> rect_fine_t;

[[nodiscard]] auto is_colliding(setting_handle_t handle, point_fine_t position) -> bool;

[[nodiscard]] auto get_colliding_setting_handle(
    point_fine_t position, const Layout& layout,
    const Selection& selection) -> std::optional<setting_handle_t>;

}  // namespace logicsim

#endif
