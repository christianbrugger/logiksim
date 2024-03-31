#include "load_save_file.h"

#include "file.h"
#include "serialize.h"

namespace logicsim {

auto serialize_circuit(const Layout& layout, SimulationConfig simulation_config)
    -> std::string {
    auto relevant_config = SimulationConfig {
        .use_wire_delay = simulation_config.use_wire_delay,
    };

    return serialize_all(layout, {}, relevant_config);
}

auto save_circuit_to_file(const Layout& layout, std::filesystem::path filename,
                          std::optional<ViewPoint> view_point,
                          std::optional<SimulationConfig> simulation_config) -> bool {
    const auto binary = serialize_all(layout, view_point, simulation_config);
    return save_file(filename, binary);
}

auto LoadFileResult::format() const -> std::string {
    return fmt::format(
        "LoadFileResult(\n"
        "  success = {},\n"
        "  editable_circuit = {},\n"
        "  view_point = {},\n"
        "  simulation_config = {}\n"
        ")",
        success, editable_circuit, view_point, simulation_config);
}

auto load_circuit_from_file(std::filesystem::path filename) -> LoadFileResult {
    const auto load_result = load_layout(load_file(filename));
    if (!load_result) {
        return LoadFileResult {};
    }

    auto editable_circuit__ = EditableCircuit {Layout {}};
    load_result->add(editable_circuit__, {InsertionMode::insert_or_discard});

    return LoadFileResult {
        .success = true,
        .editable_circuit = std::move(editable_circuit__),
        .view_point = load_result->view_point(),
        .simulation_config = load_result->simulation_config(),
    };
}

}  // namespace logicsim
