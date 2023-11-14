#ifndef LOGICSIM_VOCABULARY_CIRCUIT_WIDGET_STATE_H
#define LOGICSIM_VOCABULARY_CIRCUIT_WIDGET_STATE_H

#include "algorithm/to_underlying.h"
#include "format/struct.h"
#include "vocabulary/default_mouse_action.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>

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

    [[nodiscard]] auto operator()(const logicsim::SimulationState &obj) const noexcept
        -> uint64_t {
         return logicsim::wyhash();
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::NonInteractiveState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::NonInteractiveState &obj) const noexcept
        -> uint64_t {
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
