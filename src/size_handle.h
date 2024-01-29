#ifndef LOGIKSIM_SIZE_HANDLE_H
#define LOGIKSIM_SIZE_HANDLE_H

#include "vocabulary/point_fine.h"

#include <blend2d.h>

#include <optional>
#include <vector>

namespace logicsim {

class Layout;
class Selection;
class EditableCircuit;
struct ViewConfig;
struct PlacedElement;

struct logicitem_id_t;
struct rect_fine_t;

namespace defaults {
constexpr static inline auto size_handle_stroke_width_device = 1;  // device coordinates
constexpr static inline auto size_handle_rect_size_device = 8;     // device coordinates
}  // namespace defaults

struct size_handle_t {
    int index;
    point_fine_t point;
};

auto size_handle_positions(const Layout& layout, logicitem_id_t logicitem_id)
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

auto get_resized_element(const PlacedElement& original, size_handle_t handle, int delta)
    -> PlacedElement;

auto get_single_placed_element(const EditableCircuit& editable_circuit) -> PlacedElement;

}  // namespace logicsim

#endif
