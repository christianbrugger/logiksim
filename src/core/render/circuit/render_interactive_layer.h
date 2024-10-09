#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_INTERACTIVE_LAYER_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_INTERACTIVE_LAYER_H

#include "allocated_size/trait.h"
#include "format/struct.h"
#include "vocabulary/display_state.h"
#include "vocabulary/drawable_element.h"
#include "vocabulary/rect.h"
#include "vocabulary/segment_info.h"
#include "vocabulary/wire_id.h"

#include <concepts>
#include <optional>
#include <vector>

namespace logicsim {

class Layout;
class Selection;

struct Context;
class ImageSurface;

[[nodiscard]] auto to_element_draw_state(const Layout& layout,
                                         logicitem_id_t logicitem_id,
                                         const Selection* selection) -> ElementDrawState;

[[nodiscard]] auto to_element_draw_state(const Layout& layout,
                                         decoration_id_t decoration_id,
                                         const Selection* selection) -> ElementDrawState;

// TODO any class invariants ?
struct InteractiveLayers {
    // inserted
    std::vector<DrawableLogicItem> normal_below;
    std::vector<wire_id_t> normal_wires;
    std::vector<DrawableLogicItem> normal_above;
    std::vector<DrawableDecoration> normal_decorations;

    // uninserted
    std::vector<DrawableLogicItem> uninserted_below;
    std::vector<DrawableLogicItem> uninserted_above;
    std::vector<DrawableDecoration> uninserted_decorations;

    // selected & temporary
    std::vector<logicitem_id_t> selected_logicitems;
    std::vector<decoration_id_t> selected_decorations;
    std::vector<ordered_line_t> selected_wires;
    std::vector<segment_info_t> temporary_wires;
    // valid
    std::vector<logicitem_id_t> valid_logicitems;
    std::vector<decoration_id_t> valid_decorations;
    std::vector<ordered_line_t> valid_wires;
    // colliding
    std::vector<logicitem_id_t> colliding_logicitems;
    std::vector<decoration_id_t> colliding_decorations;
    std::vector<segment_info_t> colliding_wires;

    // bounding rects
    std::optional<rect_t> uninserted_bounding_rect;
    std::optional<rect_t> overlay_bounding_rect;

   public:
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const InteractiveLayers&) const -> bool = default;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto has_inserted() const -> bool;
    [[nodiscard]] auto has_uninserted() const -> bool;
    [[nodiscard]] auto has_overlay() const -> bool;

    auto calculate_overlay_bounding_rect() -> void;
};

static_assert(std::regular<InteractiveLayers>);

auto update_uninserted_rect(InteractiveLayers& layers, rect_t bounding_rect) -> void;
auto update_uninserted_rect(InteractiveLayers& layers, ordered_line_t line) -> void;
auto update_overlay_rect(InteractiveLayers& layers, rect_t bounding_rect) -> void;
auto update_overlay_rect(InteractiveLayers& layers, ordered_line_t line) -> void;

//
// Build
//

[[nodiscard]] auto build_interactive_layers(const Layout& layout,
                                            const Selection* selection,
                                            rect_t scene_rect) -> InteractiveLayers;

//
// Render
//

auto render_inserted(Context& ctx, const Layout& layout,
                     const InteractiveLayers& layers) -> void;

auto render_uninserted(Context& ctx, const Layout& layout,
                       const InteractiveLayers& layers, bool layer_enabled) -> void;

auto render_overlay(Context& ctx, const Layout& layout, const InteractiveLayers& layers,
                    bool layer_enabled) -> void;

auto render_interactive_layers(Context& ctx, const Layout& layout,
                               const InteractiveLayers& layers,
                               ImageSurface& surface) -> void;
}  // namespace logicsim

#endif
