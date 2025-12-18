#ifndef LOGIKSIM_SIZE_HANDLE_H
#define LOGIKSIM_SIZE_HANDLE_H

#include "core/format/struct.h"
#include "core/vocabulary/placed_element.h"
#include "core/vocabulary/point_fine.h"

#include <blend2d/blend2d.h>

#include <optional>
#include <vector>

namespace logicsim {

class Layout;
class Selection;
class EditableCircuit;
struct ViewConfig;
struct PlacedLogicItem;

struct logicitem_id_t;
struct decoration_id_t;
struct rect_fine_t;

namespace defaults {
constexpr static inline auto size_handle_stroke_width_device = 1;  // device coordinates
constexpr static inline auto size_handle_rect_size_device = 8;     // device coordinates
}  // namespace defaults

struct size_handle_t {
    int index;
    point_fine_t point;

    [[nodiscard]] auto operator==(const size_handle_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct delta_movement_t {
    int horizontal;
    int vertical;

    [[nodiscard]] auto operator==(const delta_movement_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] auto size_handle_positions(const Layout& layout,
                                         logicitem_id_t logicitem_id)
    -> std::vector<size_handle_t>;

[[nodiscard]] auto size_handle_positions(const Layout& layout,
                                         decoration_id_t decoration_id)
    -> std::vector<size_handle_t>;

[[nodiscard]] auto size_handle_positions(const Layout& layout, const Selection& selection)
    -> std::vector<size_handle_t>;

[[nodiscard]] auto size_handle_rect_px(size_handle_t handle, const ViewConfig& config)
    -> BLRect;

[[nodiscard]] auto size_handle_rect_grid(size_handle_t handle, const ViewConfig& config)
    -> rect_fine_t;

[[nodiscard]] auto get_colliding_size_handle(
    point_fine_t position, const std::vector<size_handle_t>& handle_positions,
    const ViewConfig& config) -> std::optional<size_handle_t>;

[[nodiscard]] auto get_colliding_size_handle(point_fine_t position, const Layout& layout,
                                             const Selection& selection,
                                             const ViewConfig& config)
    -> std::optional<size_handle_t>;

[[nodiscard]] auto get_resized_element(const PlacedLogicItem& original,
                                       size_handle_t handle, delta_movement_t delta)
    -> PlacedLogicItem;
[[nodiscard]] auto get_resized_element(const PlacedDecoration& original,
                                       size_handle_t handle, delta_movement_t delta)
    -> PlacedDecoration;
[[nodiscard]] auto get_resized_element(const PlacedElement& original,
                                       size_handle_t handle, delta_movement_t delta)
    -> PlacedElement;

[[nodiscard]] auto get_single_placed_element(const EditableCircuit& editable_circuit)
    -> std::optional<PlacedElement>;

}  // namespace logicsim

#endif
