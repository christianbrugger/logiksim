#ifndef LOGICSIM_VOCABULARY_CIRCUIT_WIDGET_STATE_H
#define LOGICSIM_VOCABULARY_CIRCUIT_WIDGET_STATE_H

#include "core/algorithm/to_underlying.h"
#include "core/format/container.h"
#include "core/format/struct.h"
#include "core/vocabulary/default_mouse_action.h"
#include "core/wyhash.h"

#include <ankerl/unordered_dense.h>

#include <cassert>
#include <variant>

namespace logicsim {

struct SimulationState {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SimulationState &) const -> bool = default;
};

struct NonInteractiveState {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const NonInteractiveState &) const -> bool = default;
};

struct EditingState {
    DefaultMouseAction default_mouse_action;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const EditingState &) const -> bool = default;
};

using CircuitWidgetState =
    std::variant<NonInteractiveState, SimulationState, EditingState>;

static_assert(std::regular<NonInteractiveState>);
static_assert(std::regular<SimulationState>);
static_assert(std::regular<EditingState>);
static_assert(std::regular<CircuitWidgetState>);

[[nodiscard]] auto is_simulation(const CircuitWidgetState &state) -> bool;
[[nodiscard]] auto is_non_interactive(const CircuitWidgetState &state) -> bool;
[[nodiscard]] auto is_editing_state(const CircuitWidgetState &state) -> bool;

[[nodiscard]] auto is_insert_logicitem_state(const EditingState &state) -> bool;
[[nodiscard]] auto is_insert_wire_state(const EditingState &state) -> bool;
[[nodiscard]] auto is_insert_decoration_state(const EditingState &state) -> bool;
[[nodiscard]] auto is_selection_state(const EditingState &state) -> bool;
[[nodiscard]] auto is_inserting_state(const EditingState &editing_state) -> bool;

[[nodiscard]] auto is_insert_logicitem_state(const CircuitWidgetState &state) -> bool;
[[nodiscard]] auto is_insert_wire_state(const CircuitWidgetState &state) -> bool;
[[nodiscard]] auto is_insert_decoration_state(const CircuitWidgetState &state) -> bool;
[[nodiscard]] auto is_selection_state(const CircuitWidgetState &state) -> bool;
[[nodiscard]] auto is_inserting_state(const CircuitWidgetState &state) -> bool;

namespace defaults {
constexpr inline auto selection_state =
    CircuitWidgetState {EditingState {DefaultMouseAction::selection}};
}

}  // namespace logicsim

//
// Formatter
//

template <>
struct fmt::formatter<logicsim::CircuitWidgetState> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::CircuitWidgetState &obj, fmt::format_context &ctx) const {
        const auto str = std::visit([](auto &&v) { return v.format(); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

//
// Hashes
//

template <>
struct ankerl::unordered_dense::hash<logicsim::SimulationState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::SimulationState &obj
                                  [[maybe_unused]]) const noexcept -> uint64_t {
        assert(obj == logicsim::SimulationState {});
        return logicsim::wyhash();
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::NonInteractiveState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::NonInteractiveState &obj
                                  [[maybe_unused]]) const noexcept -> uint64_t {
        assert(obj == logicsim::NonInteractiveState {});
        return logicsim::wyhash();
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::EditingState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::EditingState &obj) const noexcept
        -> uint64_t {
        const auto enum_value = logicsim::to_underlying(obj.default_mouse_action);
        return logicsim::wyhash(enum_value);
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::CircuitWidgetState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::CircuitWidgetState &obj) const noexcept
        -> uint64_t {
        const auto index = static_cast<uint64_t>(obj.index());

        const auto visitor = []<typename T>(const T &alternative) {
            return ankerl::unordered_dense::hash<T> {}(alternative);
        };
        const auto alternative_hash = std::visit(visitor, obj);

        return logicsim::wyhash_128_bit(index, alternative_hash);
    }
};

#endif
