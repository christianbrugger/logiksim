#include "core/vocabulary/allocation_info.h"

#include <fmt/core.h>

namespace logicsim {

[[nodiscard]] auto LayoutAllocInfo::format() const -> std::string {
    return std::format(
        "LayoutAllocInfo:\n"
        " logicitem_store = {}\n"
        " wire_store = {}\n"
        " decoration_store = {}",
        logicitem_store, wire_store, decoration_store);
}

[[nodiscard]] auto LayoutIndexAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto EditableCircuitAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto SimulationAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto SpatialSimulationAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto InteractiveSimulationAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto CircuitStoreAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto TextCacheAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto ContextCacheAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto CircuitRendererAllocInfo::format() const -> std::string {
    return std::format("");
}

[[nodiscard]] auto CircuitWidgetAllocInfo::format() const -> std::string {
    return circuit_store.editable_circuit.layout.format();
}

}  // namespace logicsim
