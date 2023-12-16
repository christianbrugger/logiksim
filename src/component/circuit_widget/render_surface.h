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
 * Class-invariants:
 *   + thread count in context matches render config
 *   + pixel ratio in view config is same as in qt_image
 *   + frame counter is increased for each frame rendered
 */
class RenderSurface {
   public:
    auto reset() -> void;

    [[nodiscard]] auto render_config() const -> const WidgetRenderConfig&;
    auto set_render_config(WidgetRenderConfig new_config) -> void;

    // view config
    [[nodiscard]] auto view_config() const -> const ViewConfig&;
    auto set_view_point(ViewPoint view_point) -> void;
    auto set_device_pixel_ratio(double device_pixel_ratio) -> void;

    [[nodiscard]] auto statistics() const -> SurfaceStatistics;

   public:
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
    QImage qt_image_ {};
    CircuitContext context_ {};

    // setting is only written from external setter, no internal writes
    WidgetRenderConfig render_config_ {};
    EventCounter fps_counter_ {};
};

//
// Free Functions
//

auto set_view_config_offset(RenderSurface& render_surface, point_fine_t offset) -> void;

auto set_view_config_device_scale(RenderSurface& render_surface, double device_scale)
    -> void;

auto set_optimal_render_attributes(QWidget& widget) -> void;

}  // namespace circuit_widget

}  // namespace logicsim

#endif
