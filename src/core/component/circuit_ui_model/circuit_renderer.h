#ifndef LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_CIRCUIT_RENDERER_H
#define LOGICSIM_CORE_COMPONENT_CIRCUIT_UI_MODEL_CIRCUIT_RENDERER_H

#include "core/event_counter.h"
#include "core/render/context_cache.h"
#include "core/render/image_surface.h"
#include "core/vocabulary/mouse_postion_info.h"
#include "core/vocabulary/widget_render_config.h"

#include <blend2d.h>

#include <optional>

namespace logicsim {

struct GeometryInfo;
struct ViewPoint;
struct ViewConfig;
struct CircuitRendererAllocInfo;

class Layout;
class EditableCircuit;
class SpatialSimulation;

struct MousePositionInfo;

namespace circuit_ui_model {

/**
 * @brief: Statistics of the Render Surface
 */
struct SurfaceStatistics {
    double frames_per_second;
    double pixel_scale;
    BLSize image_size;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SurfaceStatistics&) const -> bool = default;
};

static_assert(std::regular<SurfaceStatistics>);

/**
 * @brief: Renders circuit widgets in efficient manner and different modes.
 *
 * Pre-conditions:
 *   +
 *
 * Class-invariants:
 *  ???
 */
class CircuitRenderer {
   public:
    explicit CircuitRenderer();

    [[nodiscard]] auto allocation_info() const -> CircuitRendererAllocInfo;

    /**
     * @brief: Free temporary memory for layers and caches.
     */
    auto reset() -> void;

    // render config
    [[nodiscard]] auto render_config() const -> const WidgetRenderConfig&;
    auto set_render_config(WidgetRenderConfig new_config) -> void;

    // view config
    [[nodiscard]] auto view_config() const -> const ViewConfig&;
    auto set_view_point(const ViewPoint& view_point) -> void;
    auto set_device_pixel_ratio(double device_pixel_ratio) -> void;

    auto set_mouse_position_info(std::optional<MousePositionInfo> info) -> void;
    [[nodiscard]] auto statistics() const -> SurfaceStatistics;

   public:
    auto render_layout(BLImage& bl_image, const Layout& layout) -> void;
    auto render_editable_circuit(BLImage& bl_image,
                                 const EditableCircuit& editable_circuit,
                                 bool show_size_handles) -> void;
    auto render_simulation(BLImage& bl_image,
                           const SpatialSimulation& spatial_simulation) -> void;

   private:
    auto count_frame(BLSizeI image_size) -> void;

    // used for layered rendering
    ImageSurface context_surface_ {};
    // to cache SVG and Text
    ContextCache context_cache_ {cache_with_default_fonts()};

    // TODO !!! not clear what can be internally set and what not
    // TODO generate this from WidgetRenderConfig in render calls
    ContextRenderSettings context_settings_ {};
    // setting are only written from external setter, no internal writes
    WidgetRenderConfig render_config_ {};

    EventCounter fps_counter_ {};
    // to report render sizes in statistics
    BLSize last_render_size_ {};
    // to draw mouse position debug information
    std::optional<MousePositionInfo> mouse_position_info_ {};
};

//
// Free Functions
//

auto set_view_config_offset(CircuitRenderer& render_surface, point_fine_t offset) -> void;

auto set_view_config_device_scale(CircuitRenderer& render_surface,
                                  double device_scale) -> void;

/**
 * @brief: Renders the given layout.
 */
// TODO move to renderer folder
auto render_to_context(Context& ctx, ImageSurface& surface,
                       const WidgetRenderConfig& render_config,
                       const Layout& layout) -> void;

/**
 * @brief: Renders the editable circuit.
 */
// TODO move to renderer folder
auto render_to_context(Context& ctx, ImageSurface& surface,
                       const WidgetRenderConfig& render_config,
                       const EditableCircuit& editable_circuit,
                       bool show_size_handles) -> void;

/**
 * @brief: Renders the spatial simulation.
 */
// TODO move to renderer folder
auto render_to_context(Context& ctx, const WidgetRenderConfig& render_config,
                       const SpatialSimulation& spatial_simulation) -> void;

/**
 * @brief: Renders mouse position debug info.
 */
// TODO move to renderer folder
auto render_mouse_position_info(Context& ctx, const WidgetRenderConfig& render_config,
                                const std::optional<MousePositionInfo>& info) -> void;

}  // namespace circuit_ui_model

}  // namespace logicsim

#endif
