#include "core/vocabulary/circuit_ui_config.h"

namespace logicsim {

[[nodiscard]] auto CircuitUIConfig::format() const -> std::string {
    return fmt::format("CircuitUiConfig({}, {}, {})", render, simulation, state);
}

}  // namespace logicsim
