#ifndef LOGICSIM_CORE_VOCABULARY_ALLOCATION_INFO_H
#define LOGICSIM_CORE_VOCABULARY_ALLOCATION_INFO_H

#include "core/format/struct.h"

#include <concepts>
#include <optional>

namespace logicsim {

struct LayoutAllocInfo {
    std::size_t logicitem_store;
    std::size_t wire_store;
    std::size_t decoration_store;

    [[nodiscard]] auto operator==(const LayoutAllocInfo& info) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<LayoutAllocInfo>);

struct LayoutIndexAllocInfo {
    std::size_t connection_index;
    std::size_t collision_index;
    std::size_t spatial_index;
    std::size_t key_index;

    [[nodiscard]] auto operator==(const LayoutIndexAllocInfo& info) const -> bool =
                                                                                 default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<LayoutIndexAllocInfo>);

struct EditableCircuitAllocInfo {
    LayoutAllocInfo layout {};
    LayoutIndexAllocInfo index {};
    std::size_t selection_store {};
    std::size_t visible_selection {};
    std::size_t history {};

    [[nodiscard]] auto operator==(const EditableCircuitAllocInfo& info) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<EditableCircuitAllocInfo>);

struct SimulationAllocInfo {
    std::size_t schematic {};
    std::size_t simulation_queue {};

    std::size_t input_values {};
    std::size_t internal_states {};
    std::size_t input_histories {};

    [[nodiscard]] auto operator==(const SimulationAllocInfo& info) const -> bool =
                                                                                default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<SimulationAllocInfo>);

struct SpatialSimulationAllocInfo {
    std::size_t layout {};
    std::size_t line_trees {};

    SimulationAllocInfo simulation {};

    [[nodiscard]] auto operator==(const SpatialSimulationAllocInfo& info) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<SpatialSimulationAllocInfo>);

struct InteractiveSimulationAllocInfo {
    SpatialSimulationAllocInfo spatial_simulation;

    std::size_t interaction_cache {};
    std::size_t event_counter {};

    [[nodiscard]] auto operator==(const InteractiveSimulationAllocInfo& info) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<InteractiveSimulationAllocInfo>);

struct CircuitStoreAllocInfo {
    EditableCircuitAllocInfo editable_circuit;
    std::optional<InteractiveSimulationAllocInfo> interactive_simulation;

    [[nodiscard]] auto operator==(const CircuitStoreAllocInfo& info) const -> bool =
                                                                                  default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<CircuitStoreAllocInfo>);

struct TextCacheAllocInfo {
    std::size_t font_faces;
    std::size_t fonts;
    std::size_t glyph_map;

    [[nodiscard]] auto operator==(const TextCacheAllocInfo& info) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<TextCacheAllocInfo>);

struct ContextCacheAllocInfo {
    TextCacheAllocInfo text_cache;
    std::size_t svg_cache;

    [[nodiscard]] auto operator==(const ContextCacheAllocInfo& info) const -> bool =
                                                                                  default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<ContextCacheAllocInfo>);

struct CircuitRendererAllocInfo {
    std::size_t image_surface;
    ContextCacheAllocInfo context_cache;

    [[nodiscard]] auto operator==(const CircuitRendererAllocInfo& info) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<CircuitRendererAllocInfo>);

struct CircuitWidgetAllocInfo {
    CircuitStoreAllocInfo circuit_store {};

    [[nodiscard]] auto operator==(const CircuitWidgetAllocInfo& info) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::regular<CircuitWidgetAllocInfo>);
}  // namespace logicsim

#endif
