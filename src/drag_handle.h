#ifndef LOGIKSIM_DRAG_HANDLE_H
#define LOGIKSIM_DRAG_HANDLE_H

#include "editable_circuit/selection_registrar.h"
#include "editable_circuit/type.h"
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
    int index;
    point_fine_t point;
};

auto drag_handle_positions(const layout::ConstElement element)
    -> std::vector<drag_handle_t>;

auto drag_handle_positions(const Layout& layout, const Selection& selection)
    -> std::vector<drag_handle_t>;

auto drag_handle_rect_px(drag_handle_t handle_position, const ViewConfig& config)
    -> BLRect;

auto drag_handle_rect_gird(drag_handle_t handle_position, const ViewConfig& config)
    -> rect_fine_t;

auto get_colliding_handle(point_fine_t position,
                          const std::vector<drag_handle_t>& handle_positions,
                          const ViewConfig& config) -> std::optional<drag_handle_t>;

auto get_colliding_handle(point_fine_t position, const Layout& layout,
                          const Selection& selection, const ViewConfig& config)
    -> std::optional<drag_handle_t>;

namespace drag_handle {

struct logic_item_t {
    LogicItemDefinition definition;
    point_t position;
};

}  // namespace drag_handle

class MouseDragHandleLogic {
   public:
    struct Args {
        EditableCircuit& editable_circuit;
        drag_handle_t drag_handle;
    };

    MouseDragHandleLogic(Args args) noexcept;
    ~MouseDragHandleLogic();

    auto mouse_press(point_fine_t position) -> void;
    auto mouse_move(point_fine_t position) -> void;
    auto mouse_release(point_fine_t position) -> void;

   private:
    auto move_handle(point_fine_t position) -> void;
    auto temp_item_colliding() const -> bool;
    auto temp_item_exists() const -> bool;

    EditableCircuit& editable_circuit_;
    drag_handle_t drag_handle_;
    drag_handle::logic_item_t initial_logic_item_ {};

    std::optional<point_fine_t> first_position_ {};
    std::optional<int> last_delta_ {};
    selection_handle_t temp_item_ {};
};

}  // namespace logicsim

#endif
