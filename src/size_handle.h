#ifndef LOGIKSIM_SIZE_HANDLE_H
#define LOGIKSIM_SIZE_HANDLE_H

#include "editable_circuit/selection_registrar.h"
#include "vocabulary.h"
#include "vocabulary/placed_element.h"

#include <blend2d.h>

#include <optional>
#include <vector>

namespace logicsim {

class Layout;
class Selection;
class EditableCircuit;
struct ViewConfig;

namespace defaults {
constexpr static inline auto size_handle_stroke_width_device = 1;  // device coordinates
constexpr static inline auto size_handle_rect_size_device = 8;     // device coordinates
}  // namespace defaults

struct size_handle_t {
    int index;
    point_fine_t point;
};

auto size_handle_positions(const layout::ConstElement element)
    -> std::vector<size_handle_t>;

auto size_handle_positions(const Layout& layout, const Selection& selection)
    -> std::vector<size_handle_t>;

auto size_handle_rect_px(size_handle_t handle, const ViewConfig& config) -> BLRect;

auto size_handle_rect_grid(size_handle_t handle, const ViewConfig& config) -> rect_fine_t;

auto get_colliding_size_handle(point_fine_t position,
                               const std::vector<size_handle_t>& handle_positions,
                               const ViewConfig& config) -> std::optional<size_handle_t>;

auto get_colliding_size_handle(point_fine_t position, const Layout& layout,
                               const Selection& selection, const ViewConfig& config)
    -> std::optional<size_handle_t>;

class MouseSizeHandleLogic {
   public:
    struct Args {
        EditableCircuit& editable_circuit;
        size_handle_t size_handle;
    };

    MouseSizeHandleLogic(Args args) noexcept;
    ~MouseSizeHandleLogic();

    auto mouse_press(point_fine_t position) -> void;
    auto mouse_move(point_fine_t position) -> void;
    auto mouse_release(point_fine_t position) -> void;

   private:
    auto move_handle(point_fine_t position) -> void;
    auto temp_item_colliding() const -> bool;
    auto temp_item_exists() const -> bool;

    EditableCircuit& editable_circuit_;
    size_handle_t size_handle_;
    PlacedElement initial_logic_item_ {};

    std::optional<point_fine_t> first_position_ {};
    std::optional<int> last_delta_ {};
    selection_handle_t temp_item_ {};
};

}  // namespace logicsim

#endif
