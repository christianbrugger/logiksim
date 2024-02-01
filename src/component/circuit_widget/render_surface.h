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
    auto set_view_point(ViewPoint view_point) -> void;
    auto set_device_pixel_ratio(double device_pixel_ratio) -> void;

    [[nodiscard]] auto statistics() const -> SurfaceStatistics;

   public:
    class ScopedContext;

    /**
     * @brief: Sets up the painting for the given paint device and size.
     *
     * Note paintEvent can only be called within paint-event.
     *
     * If the backend store supports direct rendering it is used,
     * otherwise a QImage buffer is setup for rendering.
     */
    auto paintEvent(QWidget& widget) -> ScopedContext;

   private:
    /**
     * @brief: Sets up the painting for the given backing store and size.
     *
     * Note begin_paint() can only be used inside a paintEvent() function.
     */
    auto begin_paint(GeometryInfo geometry_info, QBackingStore* backing_store = nullptr)
        -> CircuitContext&;

    /**
     * @brief: Finalizes the painting of the context for the given paint device.
     *
     * The paint_device is only used if direct rendering was not possible.
     *
     * Note end_paint() can only be used inside a paintEvent() function.
     *
     * This function needs to be called at all times, even if an exception occurs.
     */
    auto end_paint(QPaintDevice& paint_device) -> void;

   private:
    QImage qt_image_ {};
    CircuitContext context_ {};

    // setting is only written from external setter, no internal writes
    WidgetRenderConfig render_config_ {};
    EventCounter fps_counter_ {};
    BLSize last_render_size_ {};
};

class RenderSurface::ScopedContext {
   public:
    explicit ScopedContext(RenderSurface& render_surface, QPaintDevice& paint_device,
                           GeometryInfo geometry_info,
                           QBackingStore* backing_store = nullptr);
    ~ScopedContext();

    [[nodiscard]] auto context() -> CircuitContext&;

   private:
    RenderSurface& render_surface_;
    QPaintDevice& paint_device_;
    CircuitContext& context_;
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
auto render_to_context(CircuitContext& context, const WidgetRenderConfig& render_config,
                       const Layout& layout) -> void;

/**
 * @brief: Renders the editable circuit.
 */
// TODO move to renderer folder
auto render_to_context(CircuitContext& context, const WidgetRenderConfig& render_config,
                       const EditableCircuit& editable_circuit, bool show_size_handles)
    -> void;

/**
 * @brief: Renders the spatial simulation.
 */
// TODO move to renderer folder
auto render_to_context(CircuitContext& context, const WidgetRenderConfig& render_config,
                       const SpatialSimulation& spatial_simulation) -> void;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
