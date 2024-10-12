#ifndef LOGICSIM_LOAD_SAVE_FILE_H
#define LOGICSIM_LOAD_SAVE_FILE_H

#include "editable_circuit.h"
#include "exception/load_error.h"
#include "vocabulary/simulation_config.h"
#include "vocabulary/view_config.h"

#include <tl/expected.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace logicsim {

class Layout;
struct ViewPoint;
struct SimulationConfig;

// TODO remove once we have history
auto serialize_circuit(const Layout &layout,
                       SimulationConfig simulation_config) -> std::string;

auto save_circuit_to_file(const Layout &layout, const std::filesystem::path &filename,
                          std::optional<ViewPoint> view_point,
                          std::optional<SimulationConfig> simulation_config) -> bool;

struct LoadFileResult {
    EditableCircuit editable_circuit {};
    ViewPoint view_point {};
    SimulationConfig simulation_config {};

    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] auto load_circuit_from_file(const std::filesystem::path &filename)
    -> tl::expected<LoadFileResult, LoadError>;

}  // namespace logicsim

#endif
