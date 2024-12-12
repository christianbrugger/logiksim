#include "core/vocabulary/allocation_info.h"

#include "core/algorithm/indent.h"

#include <fmt/core.h>

namespace logicsim {

namespace {

[[nodiscard]] auto aindent(const auto& obj) -> std::string {
    return indent(obj.format(), 2);
}

}  // namespace

auto Byte::format() const -> std::string {
    if (value == 0) {
        return "0";
    }
    if (value < 1000 * 1000) {
        return fmt::format("{:.3g} KB", static_cast<double>(value) / 1000.);
    }
    return fmt::format("{:.3g} MB", static_cast<double>(value) / 1000. / 1000.);
}

auto LayoutAllocInfo::format() const -> std::string {
    return fmt::format(
        "Layout:\n"
        "  logicitem_store   {}\n"
        "  wire_store        {}\n"
        "  decoration_store  {}",
        logicitem_store, wire_store, decoration_store);
}

auto LayoutIndexAllocInfo::format() const -> std::string {
    return fmt::format(
        "Index:\n"
        "  connection_index = {}\n"
        "  collision_index = {}\n"
        "  spatial_index = {}\n"
        "  key_index = {}",
        connection_index, collision_index, spatial_index, key_index);
}

auto EditableCircuitAllocInfo::format() const -> std::string {
    return fmt::format(
        "EditableCircuit:\n"
        "{}\n"
        "{}\n"
        "  selection_store = {}\n"
        "  visible_selection = {}\n"
        "  history = {}",
        aindent(layout), aindent(index), selection_store, visible_selection, history);
}

auto SimulationAllocInfo::format() const -> std::string {
    return fmt::format(
        "Simulation:\n"
        "  schematic = {}\n"
        "  simulation_queue = {}\n"
        "  input_values = {}\n"
        "  internal_states = {}\n"
        "  input_histories = {}",
        schematic, simulation_queue, input_values, internal_states, input_histories);
}

auto SpatialSimulationAllocInfo::format() const -> std::string {
    return fmt::format(
        "SpatialSimulation:\n"
        "{}\n"
        "  svg_cache = {}\n"
        "{}",
        aindent(layout), line_trees, simulation);
}

auto InteractiveSimulationAllocInfo::format() const -> std::string {
    return fmt::format(
        "InteractiveSimulation:\n"
        "{}\n"
        "  svg_cache = {}\n"
        "  svg_cache = {}",
        aindent(spatial_simulation), interaction_cache, event_counter);
}

auto CircuitStoreAllocInfo::format() const -> std::string {
    const auto sim_str =
        interactive_simulation
            ? fmt::format("\n{}", aindent(interactive_simulation.value()))
            : "";

    return fmt::format(
        "CircuitStore:\n"
        "{}{}",
        aindent(editable_circuit), sim_str);
}

auto TextCacheAllocInfo::format() const -> std::string {
    return fmt::format(
        "EditableCircuit:\n"
        "  faces = {}\n"
        "  fonts = {}\n"
        "  glyph_map = {}",
        faces, fonts, glyph_map);
}

auto ContextCacheAllocInfo::format() const -> std::string {
    return fmt::format(
        "Cache:\n"
        "{}\n"
        "  svg_cache = {}",
        aindent(text_cache), svg_cache);
}

auto CircuitRendererAllocInfo::format() const -> std::string {
    return fmt::format(
        "Renderer:\n"
        "  image_surface = {}\n"
        "{}",
        image_surface, aindent(context_cache));
}

auto CircuitWidgetAllocInfo::format() const -> std::string {
    return fmt::format(
        "CircuitWidget:\n"
        "{}\n"
        "{}\n"
        "  collection_time = {:.3f} ms",
        aindent(circuit_store), aindent(circuit_renderer),
        collection_time.count() * 1000);
}

}  // namespace logicsim
