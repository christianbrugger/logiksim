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

class EditableCircuit;
class SpatialSimulation;

namespace circuit_widget {

class RenderSurface {
   public:
    auto set_render_config(WidgetRenderConfig new_config) -> void;
    auto clear_caches() -> void;

    // events we depend on to be called
    auto resizeEvent(QWidget& widget, QResizeEvent* event_) -> void;
    auto paintEvent(QWidget& widget, QPaintEvent* event_,
                    EditableCircuit* editable_circuit, SpatialSimulation* spatial_simulation,
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
