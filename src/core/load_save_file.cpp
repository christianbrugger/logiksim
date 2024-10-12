#include "load_save_file.h"

#include "file.h"
#include "serialize.h"

namespace logicsim {

auto serialize_circuit(const Layout& layout,
                       SimulationConfig simulation_config) -> std::string {
    auto relevant_config = SimulationConfig {
        .use_wire_delay = simulation_config.use_wire_delay,
    };

    return serialize_all(layout, {}, relevant_config, SaveFormat::gzip);
}

auto save_circuit_to_file(const Layout& layout, const std::filesystem::path& filename,
                          std::optional<ViewPoint> view_point,
                          std::optional<SimulationConfig> simulation_config) -> bool {
    const auto binary =
        serialize_all(layout, view_point, simulation_config, SaveFormat::gzip);
    return save_file(filename, binary);
}

auto LoadFileResult::format() const -> std::string {
    return fmt::format(
        "LoadFileResult(\n"
        "  editable_circuit = {},\n"
        "  view_point = {},\n"
        "  simulation_config = {}\n"
        ")",
        editable_circuit, view_point, simulation_config);
}

namespace {

[[nodiscard]] auto to_load_file_result(const serialize::LoadLayoutResult& load_result)
    -> LoadFileResult {
    auto editable_circuit = EditableCircuit {};
    load_result.add_to(editable_circuit, {InsertionMode::insert_or_discard});

    return LoadFileResult {
        .editable_circuit = std::move(editable_circuit),
        .view_point = load_result.view_point(),
        .simulation_config = load_result.simulation_config(),
    };
}

}  // namespace

auto load_circuit_from_file(const std::filesystem::path& filename)
    -> tl::expected<LoadFileResult, LoadError> {
    return load_layout(load_file(filename)).transform(to_load_file_result);
}

}  // namespace logicsim
