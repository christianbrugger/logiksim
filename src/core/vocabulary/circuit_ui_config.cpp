#include "core/vocabulary/circuit_ui_config.h"

namespace logicsim {

[[nodiscard]] auto CircuitUiConfig::format() const -> std::string {
    return fmt::format("CircuitUiConfig({}, {}, {})", render_config, simulation_config,
                       circuit_state);
}

}  // namespace logicsim
