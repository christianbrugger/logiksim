#ifndef LOGICSIM_LOAD_SAVE_FILE_H
#define LOGICSIM_LOAD_SAVE_FILE_H

#include "editable_circuit.h"
#include "vocabulary/simulation_config.h"
#include "vocabulary/view_config.h"

#include <filesystem>
#include <optional>
#include <string>

namespace logicsim {

class Layout;
struct ViewPoint;
struct SimulationConfig;

// TODO remove once we have history
auto serialize_circuit(const Layout &layout, SimulationConfig simulation_config)
    -> std::string;

auto save_circuit_to_file(const Layout &layout, std::filesystem::path filename,
                          std::optional<ViewPoint> view_point,
                          std::optional<SimulationConfig> simulation_config) -> bool;

struct LoadFileResult {
    bool success {false};
    EditableCircuit editable_circuit {};
    ViewPoint view_point {};
    SimulationConfig simulation_config {};

    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] auto load_circuit_from_file(std::filesystem::path filename)
    -> LoadFileResult;

}  // namespace logicsim

#endif
