#ifndef LOGICSIM_WIDGET_CIRCUIT_WIDGET_BASE_H
#define LOGICSIM_WIDGET_CIRCUIT_WIDGET_BASE_H

#include "gui/widget/render_widget.h"

#include "core/vocabulary/circuit_widget_state.h"
#include "core/vocabulary/history_status.h"
#include "core/vocabulary/simulation_config.h"
#include "core/vocabulary/widget_render_config.h"

namespace logicsim {

class CircuitWidgetBase : public RenderWidget {
    Q_OBJECT

   public:
    using RenderWidget::RenderWidget;

    Q_SIGNAL void render_config_changed(logicsim::WidgetRenderConfig new_config);
    Q_SIGNAL void simulation_config_changed(logicsim::SimulationConfig new_config);
    Q_SIGNAL void circuit_state_changed(logicsim::CircuitWidgetState new_state);
    Q_SIGNAL void history_status_changed(logicsim::HistoryStatus new_status);

   protected:
    auto emit_render_config_changed(WidgetRenderConfig new_config) -> void;
    auto emit_simulation_config_changed(SimulationConfig new_config) -> void;
    auto emit_circuit_state_changed(CircuitWidgetState new_state) -> void;
    auto emit_history_status_changed(HistoryStatus new_status) -> void;
};

}  // namespace logicsim

#endif
