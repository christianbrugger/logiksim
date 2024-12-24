#ifndef LOGICSIM_CORE_VOCABULARY_ALLOCATION_INFO_H
#define LOGICSIM_CORE_VOCABULARY_ALLOCATION_INFO_H

#include "core/algorithm/numeric.h"
#include "core/format/struct.h"

#include <chrono>
#include <concepts>
#include <optional>

namespace logicsim {

struct Byte {
    std ::size_t value {};

    [[nodiscard]] auto operator==(const Byte&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] constexpr auto operator+(Byte a, Byte b) -> Byte;

static_assert(std::regular<Byte>);

struct LayoutAllocInfo {
    Byte logicitem_store {};
    Byte wire_store {};
    Byte decoration_store {};

    [[nodiscard]] auto operator==(const LayoutAllocInfo&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<LayoutAllocInfo>);

struct LayoutIndexAllocInfo {
    Byte connection_index {};
    Byte collision_index {};
    Byte spatial_index {};
    Byte key_index {};

    [[nodiscard]] auto operator==(const LayoutIndexAllocInfo&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<LayoutIndexAllocInfo>);

struct CircuitDataAllocInfo {
    LayoutAllocInfo layout {};
    LayoutIndexAllocInfo index {};
    Byte selection_store {};
    Byte visible_selection {};
    Byte history {};
    std::optional<Byte> messages {};
    std::optional<Byte> message_validator {};

    [[nodiscard]] auto operator==(const CircuitDataAllocInfo&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<CircuitDataAllocInfo>);

struct SimulationAllocInfo {
    Byte schematic {};
    Byte simulation_queue {};

    Byte input_values {};
    Byte internal_states {};
    Byte input_histories {};

    [[nodiscard]] auto operator==(const SimulationAllocInfo&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<SimulationAllocInfo>);

struct SpatialSimulationAllocInfo {
    LayoutAllocInfo layout {};
    Byte line_trees {};
    SimulationAllocInfo simulation {};

    [[nodiscard]] auto operator==(const SpatialSimulationAllocInfo&) const -> bool =
                                                                                  default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<SpatialSimulationAllocInfo>);

struct InteractiveSimulationAllocInfo {
    SpatialSimulationAllocInfo spatial_simulation {};
    Byte interaction_index {};
    Byte event_counter {};  // TODO ???

    [[nodiscard]] auto operator==(const InteractiveSimulationAllocInfo&) const
        -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<InteractiveSimulationAllocInfo>);

struct CircuitStoreAllocInfo {
    CircuitDataAllocInfo editable_circuit {};
    std::optional<InteractiveSimulationAllocInfo> interactive_simulation {};

    [[nodiscard]] auto operator==(const CircuitStoreAllocInfo&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<CircuitStoreAllocInfo>);

struct TextCacheAllocInfo {
    Byte faces {};  // TODO ???
    Byte fonts {};  // TODO ???
    Byte glyph_map {};

    [[nodiscard]] auto operator==(const TextCacheAllocInfo&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<TextCacheAllocInfo>);

struct ContextCacheAllocInfo {
    TextCacheAllocInfo text_cache {};
    Byte svg_cache {};  // TODO ???

    [[nodiscard]] auto operator==(const ContextCacheAllocInfo&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<ContextCacheAllocInfo>);

struct CircuitRendererAllocInfo {
    Byte image_surface {};
    ContextCacheAllocInfo context_cache {};

    [[nodiscard]] auto operator==(const CircuitRendererAllocInfo&) const -> bool =
                                                                                default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<CircuitRendererAllocInfo>);

struct CircuitWidgetAllocInfo {
    CircuitStoreAllocInfo circuit_store {};
    CircuitRendererAllocInfo circuit_renderer {};

    std::chrono::duration<double> collection_time {};

    [[nodiscard]] auto operator==(const CircuitWidgetAllocInfo&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto total() const -> Byte;
};

static_assert(std::regular<CircuitWidgetAllocInfo>);

//
// Implementation
//

[[nodiscard]] constexpr auto operator+(Byte a, Byte b) -> Byte {
    return Byte {checked_add(a.value, b.value)};
}

}  // namespace logicsim

#endif
