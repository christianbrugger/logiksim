#ifndef LOGICSIM_CIRCUIT_WIDGET_BASE_H
#define LOGICSIM_CIRCUIT_WIDGET_BASE_H

#include "vocabulary/circuit_widget_state.h"
#include "vocabulary/simulation_config.h"
#include "vocabulary/widget_render_config.h"

#include <QWidget>

namespace logicsim {

class CircuitWidgetBase : public QWidget {
    Q_OBJECT

   public:
    using QWidget::QWidget;

    Q_SIGNAL void render_config_changed(logicsim::WidgetRenderConfig new_config);
    Q_SIGNAL void simulation_config_changed(logicsim::SimulationConfig new_config);
    Q_SIGNAL void circuit_state_changed(logicsim::CircuitWidgetState new_state);

   protected:
    auto emit_render_config_changed(WidgetRenderConfig new_config) -> void;
    auto emit_simulation_config_changed(SimulationConfig new_config) -> void;
    auto emit_circuit_state_changed(CircuitWidgetState new_state) -> void;
};

}  // namespace logicsim

#endif
