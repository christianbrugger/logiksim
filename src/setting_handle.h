#ifndef LOGIKSIM_SETTING_HANDLE
#define LOGIKSIM_SETTING_HANDLE

#include "resource.h"
#include "vocabulary.h"

#include <blend2d.h>

#include <QWidget>

#include <optional>

namespace logicsim {
class Layout;
class Selection;
class EditableCircuit;
class ViewConfig;

namespace defaults {
constexpr static inline auto setting_handle_size = grid_fine_t {1.0};
constexpr static inline auto setting_handle_margin = grid_fine_t {0.1};
}  // namespace defaults

struct setting_handle_t {
    point_fine_t position;
    icon_t icon;
};

auto setting_handle_position(const Layout& layout, element_id_t element_id)
    -> std::optional<setting_handle_t>;

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t>;

auto setting_handle_rect(setting_handle_t handle) -> rect_fine_t;

auto is_colliding(setting_handle_t handle, point_fine_t position) -> bool;

auto get_colliding_setting_handle(point_fine_t position, const Layout& layout,
                                  const Selection& selection)
    -> std::optional<setting_handle_t>;

//
// Mouse Logic
//

class MouseSettingHandleLogic {
   public:
    struct Args {
        EditableCircuit& editable_circuit;
        setting_handle_t setting_handle;
        QWidget* parent;
    };

    MouseSettingHandleLogic(Args args) noexcept;

    auto mouse_press(point_fine_t position) -> void;
    auto mouse_release(point_fine_t position) -> void;

   private:
    EditableCircuit& editable_circuit_;
    setting_handle_t setting_handle_;
    QWidget* parent_;

    std::optional<point_fine_t> first_position_ {};
};

// Setting Widgets

}  // namespace logicsim

#endif