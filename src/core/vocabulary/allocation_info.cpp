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
    if (value < std::size_t {1000} * std::size_t {1000}) {
        return fmt::format("{:.3f} KB", static_cast<double>(value) / 1000.);
    }
    return fmt::format("{:.3f} MB", static_cast<double>(value) / 1000. / 1000.);
}

auto LayoutAllocInfo::format() const -> std::string {
    return fmt::format(
        "Layout ({}):\n"
        "  logicitem_store:  {}\n"
        "  wire_store:       {}\n"
        "  decoration_store: {}",
        total(), logicitem_store, wire_store, decoration_store);
}

auto LayoutAllocInfo::total() const -> Byte {
    return logicitem_store +  //
           wire_store +       //
           decoration_store;
}

auto LayoutIndexAllocInfo::format() const -> std::string {
    return fmt::format(
        "Index ({}):\n"
        "  connection_index: {}\n"
        "  collision_index:  {}\n"
        "  spatial_index:    {}\n"
        "  key_index:        {}",
        total(), connection_index, collision_index, spatial_index, key_index);
}

auto LayoutIndexAllocInfo::total() const -> Byte {
    return connection_index +  //
           collision_index +   //
           spatial_index +     //
           key_index;
}

auto CircuitDataAllocInfo::format() const -> std::string {
    return fmt::format(
        "EditableCircuit ({}):\n"
        "{}\n"
        "{}\n"
        "  selection_store:   {}\n"
        "  visible_selection: {}\n"
        "  history:           {}\n"
        "  messages:          {}\n"
        "  message_validator: {}",
        total(), aindent(layout), aindent(index), selection_store, visible_selection,
        history, messages, message_validator);
}

auto CircuitDataAllocInfo::total() const -> Byte {
    return layout.total() +     //
           index.total() +      //
           selection_store +    //
           visible_selection +  //
           history +            //
           messages +           //
           message_validator;
}

auto SimulationAllocInfo::format() const -> std::string {
    return fmt::format(
        "Simulation ({}):\n"
        "  schematic:        {}\n"
        "  simulation_queue: {}\n"
        "  input_values:     {}\n"
        "  internal_states:  {}\n"
        "  input_histories:  {}",
        total(), schematic, simulation_queue, input_values, internal_states,
        input_histories);
}

auto SimulationAllocInfo::total() const -> Byte {
    return schematic +         //
           simulation_queue +  //
           input_values +      //
           internal_states +   //
           input_histories;
}

auto SpatialSimulationAllocInfo::format() const -> std::string {
    return fmt::format(
        "SpatialSimulation ({}):\n"
        "{}\n"
        "  line_trees: {}\n"
        "{}",
        total(), aindent(layout), line_trees, aindent(simulation));
}

auto SpatialSimulationAllocInfo::total() const -> Byte {
    return layout.total() +  //
           line_trees +      //
           simulation.total();
}

auto InteractiveSimulationAllocInfo::format() const -> std::string {
    return fmt::format(
        "InteractiveSimulation ({}):\n"
        "{}\n"
        "  interaction_cache: {}\n"
        "  event_counter:     {}",
        total(), aindent(spatial_simulation), interaction_cache, event_counter);
}

auto InteractiveSimulationAllocInfo::total() const -> Byte {
    return spatial_simulation.total() +  //
           interaction_cache +           //
           event_counter;
}

auto CircuitStoreAllocInfo::format() const -> std::string {
    const auto sim_str =
        interactive_simulation
            ? fmt::format("\n{}", aindent(interactive_simulation.value()))
            : "";

    return fmt::format(
        "CircuitStore ({}):\n"
        "{}{}",
        total(), aindent(editable_circuit), sim_str);
}

auto CircuitStoreAllocInfo::total() const -> Byte {
    return editable_circuit.total() +  //
           (interactive_simulation ? interactive_simulation->total() : Byte {});
}

auto TextCacheAllocInfo::format() const -> std::string {
    return fmt::format(
        "TextCache ({}):\n"
        "  faces:     {}\n"
        "  fonts:     {}\n"
        "  glyph_map: {}",
        total(), faces, fonts, glyph_map);
}

auto TextCacheAllocInfo::total() const -> Byte {
    return faces +  //
           fonts +  //
           glyph_map;
}

auto ContextCacheAllocInfo::format() const -> std::string {
    return fmt::format(
        "ContextCache ({}):\n"
        "{}\n"
        "  svg_cache: {}",
        total(), aindent(text_cache), svg_cache);
}

auto ContextCacheAllocInfo::total() const -> Byte {
    return text_cache.total() +  //
           svg_cache;
}

auto CircuitRendererAllocInfo::format() const -> std::string {
    return fmt::format(
        "Renderer ({}):\n"
        "  image_surface: {}\n"
        "{}",
        total(), image_surface, aindent(context_cache));
}

auto CircuitRendererAllocInfo::total() const -> Byte {
    return image_surface +  //
           context_cache.total();
}

auto CircuitWidgetAllocInfo::format() const -> std::string {
    const auto collection_time_ms = collection_time.count() * 1000;

    return fmt::format(
        "CircuitWidget ({}):\n"
        "{}\n"
        "{}\n"
        "\n"
        "collection_time: {:.3f} ms",
        total(), aindent(circuit_store), aindent(circuit_renderer), collection_time_ms);
}

auto CircuitWidgetAllocInfo::total() const -> Byte {
    return circuit_store.total() +  //
           circuit_renderer.total();
}

}  // namespace logicsim
