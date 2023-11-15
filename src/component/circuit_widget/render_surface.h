#ifndef LOGICSIM_COMPONENT_CIRCUIT_WIDGET_RENDER_SURFACE_H
#define LOGICSIM_COMPONENT_CIRCUIT_WIDGET_RENDER_SURFACE_H

#include "event_counter.h"
#include "render_circuit.h"
#include "vocabulary/widget_render_config.h"

#include <QImage>
#include <QPaintEvent>
#include <QResizeEvent>

class QWidget;

namespace logicsim {

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
 * Note this component also holds the view config.
 */
class RenderSurface {
   public:
    auto set_render_config(WidgetRenderConfig new_config) -> void;
    auto clear_caches() -> void;

    // view config
    [[nodiscard]] auto view_config() const -> const ViewConfig&;
    auto set_view_config_offset(point_fine_t offset) -> void;
    auto set_view_config_device_scale(double scale) -> void;
    auto set_view_point(ViewPoint view_point) -> void;

    [[nodiscard]] auto statistics() const -> SurfaceStatistics;

   public:
    // events we depend on to be called
    auto resizeEvent(QWidget& widget) -> void;

    /**
     * @brief: Renders the circuit or simulation.
     *
     * Only one object is rendered with the following priority:
     *     1) editable_circuit
     *     2) spatial_simulation
     *     3) layout
     *
     * Note that handles are only rendered when editable_circuit is provided,
     * as it requires a selection.
     *
     * Note that caches are only rendered if editable_circuit is provided.
     */
    auto paintEvent(QWidget& widget, const EditableCircuit* editable_circuit,
                    const SpatialSimulation* spatial_simulation, const Layout* layout,
                    bool show_size_handles) -> void;

   private:
    // Can only be called inside of paintEvent
    auto init_surface(QWidget& widget) -> void;
    auto _init_direct_rendering(QWidget& widget) -> bool;
    auto _init_buffered_rendering(QWidget& widget) -> void;

   private:
    qreal last_pixel_ratio_ {-1};
    QImage qt_image_ {};
    CircuitContext context_ {};
    bool is_initialized_ {false};

    // setting is only written from external setter, no internal writes
    WidgetRenderConfig render_config_ {};
    EventCounter fps_counter_ {};
};

}  // namespace circuit_widget

}  // namespace logicsim

#endif
