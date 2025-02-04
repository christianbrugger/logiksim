#ifndef LOGICSIM_CORE_VOCABULARY_CIRCUIT_UI_CONFIG_H
#define LOGICSIM_CORE_VOCABULARY_CIRCUIT_UI_CONFIG_H

#include "core/format/struct.h"
#include "core/vocabulary/circuit_widget_state.h"
#include "core/vocabulary/simulation_config.h"
#include "core/vocabulary/widget_render_config.h"

namespace logicsim {

struct CircuitUiConfig {
    WidgetRenderConfig render_config {};
    SimulationConfig simulation_config {};
    CircuitWidgetState circuit_state {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const CircuitUiConfig &) const -> bool = default;
};

}  // namespace logicsim

#endif
