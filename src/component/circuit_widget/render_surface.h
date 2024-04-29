#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_RENDER_SURFACE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_RENDER_SURFACE_H

#include "event_counter.h"
#include "render_circuit.h"
#include "vocabulary/widget_render_config.h"

#include <QImage>
#include <QPaintEvent>
#include <QResizeEvent>

class QWidget;
class QBackingStore;

namespace logicsim {

struct GeometryInfo;
struct ViewConfig;
class EditableCircuit;
class SpatialSimulation;

namespace circuit_widget {

/**
 * @brief: Statistics of the Render Surface
 */
struct SurfaceStatistics {
    double frames_per_second;
    double pixel_scale;
    BLSize image_size;
    bool uses_direct_rendering;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SurfaceStatistics&) const -> bool = default;
};

static_assert(std::regular<SurfaceStatistics>);

/**
 * @brief: Maintains the render buffers of the Circuit Widget for render tasks.
 *
 * Pre-conditions:
 *   +
 *
 * Class-invariants:
 *  ???
 */
class RenderSurface {
   public:
    /**
     * @brief: Free temporary memory for layers and fonts.
     */
    auto reset() -> void;

    // render config
    [[nodiscard]] auto render_config() const -> const WidgetRenderConfig&;
    auto set_render_config(WidgetRenderConfig new_config) -> void;

    // view config
    [[nodiscard]] auto view_config() const -> const ViewConfig&;
    auto set_view_point(const ViewPoint& view_point) -> void;
    auto set_device_pixel_ratio(double device_pixel_ratio) -> void;

    [[nodiscard]] auto statistics() const -> SurfaceStatistics;

   public:
    /**
     * @brief: TODO
     *
     * Note paintEvent can only be called within paint-event.
     *
     * If the backend store supports direct rendering it is used,
     * otherwise a QImage buffer is setup for rendering.
     */
    auto paintEvent(
        QWidget& widget,
        std::function<void(Context&, ImageSurface&, CircuitLayers&)> render_function)
        -> void;

   private:
    // used when backing store is not directly writable
    QImage qt_image_ {};
    // used for layered rendering
    ImageSurface context_surface_ {};
    // to cache SVG and Text
    // TODO remove dependency of FontFaces in this file
    ContextCache context_cache_ {FontFaces {get_default_font_locations()}};
    // TODO !!! remove
    CircuitLayers context_layers_ {};

    // TODO not clear what can be internally set and what not
    ContextRenderSettings context_settings_ {};
    // setting are only written from external setter, no internal writes
    WidgetRenderConfig render_config_ {};

    EventCounter fps_counter_ {};
    // to accurately report render sizes in statistics
    BLSize last_render_size_ {};
};

//
// Free Functions
//

auto set_view_config_offset(RenderSurface& render_surface, point_fine_t offset) -> void;

auto set_view_config_device_scale(RenderSurface& render_surface, double device_scale)
    -> void;

auto set_optimal_render_attributes(QWidget& widget) -> void;

/**
 * @brief: Renders the given layout.
 */
// TODO move to renderer folder
auto render_to_context(Context& ctx, ImageSurface& surface, InteractiveLayers& layers,
                       const WidgetRenderConfig& render_config, const Layout& layout)
    -> void;

/**
 * @brief: Renders the editable circuit.
 */
// TODO move to renderer folder
auto render_to_context(Context& ctx, ImageSurface& surface, InteractiveLayers& layers,
                       const WidgetRenderConfig& render_config,
                       const EditableCircuit& editable_circuit, bool show_size_handles)
    -> void;

/**
 * @brief: Renders the spatial simulation.
 */
// TODO move to renderer folder
auto render_to_context(Context& ctx, SimulationLayers& layers,
                       const WidgetRenderConfig& render_config,
                       const SpatialSimulation& spatial_simulation) -> void;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
