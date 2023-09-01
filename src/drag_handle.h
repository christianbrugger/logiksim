#ifndef LOGIKSIM_DRAG_HANDLE_H
#define LOGIKSIM_DRAG_HANDLE_H

#include "vocabulary.h"

#include <blend2d.h>

#include <optional>
#include <vector>

namespace logicsim {

namespace layout {
template <bool Const>
class ElementTemplate;
using ConstElement = ElementTemplate<true>;
}  // namespace layout

class Layout;
class Selection;
class EditableCircuit;
class ViewConfig;

namespace defaults {
constexpr static inline auto drag_handle_stroke_width_device = 1;  // device coordinates
constexpr static inline auto drag_handle_rect_size_device = 8;     // device coordinates
}  // namespace defaults

struct drag_handle_t {
    point_fine_t value;
};

auto drag_handle_positions(const layout::ConstElement element)
    -> std::vector<drag_handle_t>;

auto drag_handle_positions(const Layout& layout, const Selection& selection)
    -> std::vector<drag_handle_t>;

auto drag_handle_rect_px(drag_handle_t handle_position, const ViewConfig& config)
    -> BLRect;

auto drag_handle_rect_gird(drag_handle_t handle_position, const ViewConfig& config)
    -> rect_fine_t;

auto is_drag_handle_colliding(point_fine_t position,
                              const std::vector<drag_handle_t>& handle_positions,
                              const ViewConfig& config) -> bool;

auto is_drag_handle_colliding(point_fine_t position, const Layout& layout,
                              const Selection& selection, const ViewConfig& config)
    -> bool;

class MouseDragHandleLogic {
   public:
    struct Args {
        EditableCircuit& editable_circuit;
    };

    MouseDragHandleLogic(Args args) noexcept;

    auto mouse_press(point_fine_t position) -> void;
    auto mouse_move(point_fine_t position) -> void;
    auto mouse_release(point_fine_t position) -> void;

   private:
    EditableCircuit& editable_circuit_;
    std::optional<point_fine_t> last_position_ {};
};

}  // namespace logicsim

#endif
