#ifndef LOGICSIM_CIRCUIT_WIDGET_BASE_H
#define LOGICSIM_CIRCUIT_WIDGET_BASE_H

#include "algorithm/to_underlying.h"
#include "format/enum.h"
#include "format/struct.h"
#include "vocabulary/time_rate.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>
#include <fmt/core.h>

#include <QWidget>

#include <variant>

namespace logicsim {

namespace circuit_widget {

//
// State
//

struct SimulationState {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SimulationState &) const -> bool = default;
};

struct NonInteractiveState {
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const NonInteractiveState &) const -> bool = default;
};

enum class DefaultMouseAction {
    selection,
    insert_wire,

    insert_button,
    insert_led,
    insert_display_number,
    insert_display_ascii,

    insert_and_element,
    insert_or_element,
    insert_xor_element,
    insert_nand_element,
    insert_nor_element,

    insert_buffer_element,
    insert_inverter_element,
    insert_flipflop_jk,
    insert_latch_d,
    insert_flipflop_d,
    insert_flipflop_ms_d,

    insert_clock_generator,
    insert_shift_register,
};

struct EditingState {
    DefaultMouseAction default_mouse_action;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const EditingState &) const -> bool = default;
};

using CircuitState = std::variant<NonInteractiveState, SimulationState, EditingState>;

static_assert(std::regular<NonInteractiveState>);
static_assert(std::regular<SimulationState>);
static_assert(std::regular<EditingState>);
static_assert(std::regular<CircuitState>);

[[nodiscard]] auto is_simulation(const CircuitState &state) -> bool;

}  // namespace circuit_widget

template <>
auto format(circuit_widget::DefaultMouseAction action) -> std::string;

//
// Configs
//

namespace circuit_widget {

struct RenderConfig {
    bool do_benchmark {false};
    bool show_circuit {true};
    bool show_collision_cache {false};
    bool show_connection_cache {false};
    bool show_selection_cache {false};

    double zoom_level {1};

    int thread_count {4};
    bool direct_rendering {true};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const RenderConfig &) const -> bool = default;
};

struct SimulationConfig {
    time_rate_t simulation_time_rate {10us};
    bool use_wire_delay {true};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const SimulationConfig &) const -> bool = default;
};

}  // namespace circuit_widget

class CircuitWidgetBase : public QWidget {
    Q_OBJECT

   public:
    using RenderConfig = circuit_widget::RenderConfig;
    using SimulationConfig = circuit_widget::SimulationConfig;
    using CircuitState = circuit_widget::CircuitState;

   public:
    using QWidget::QWidget;

    Q_SIGNAL void render_config_changed(RenderConfig new_config);
    Q_SIGNAL void simulation_config_changed(SimulationConfig new_config);
    Q_SIGNAL void circuit_state_changed(CircuitState new_state);

   protected:
    auto emit_render_config_changed(RenderConfig new_config) -> void;
    auto emit_simulation_config_changed(SimulationConfig new_config) -> void;
    auto emit_circuit_state_changed(CircuitState new_state) -> void;
};

}  // namespace logicsim

//
// Formatter
//

template <>
struct fmt::formatter<logicsim::circuit_widget::CircuitState> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    auto format(const logicsim::circuit_widget::CircuitState &obj,
                fmt::format_context &ctx) const {
        const auto str = std::visit([](auto &&v) { return v.format(); }, obj);
        return fmt::format_to(ctx.out(), "{}", str);
    }
};

//
// Hashes
//

template <>
struct ankerl::unordered_dense::hash<logicsim::circuit_widget::SimulationState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(
        const logicsim::circuit_widget::SimulationState &obj) const noexcept -> uint64_t {
        return logicsim::wyhash();
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::circuit_widget::NonInteractiveState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(
        const logicsim::circuit_widget::NonInteractiveState &obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash();
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::circuit_widget::EditingState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(
        const logicsim::circuit_widget::EditingState &obj) const noexcept -> uint64_t {
        const auto enum_value = logicsim::to_underlying(obj.default_mouse_action);
        return logicsim::wyhash(enum_value);
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::circuit_widget::CircuitState> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(
        const logicsim::circuit_widget::CircuitState &obj) const noexcept -> uint64_t {
        const auto index = static_cast<uint64_t>(obj.index());

        const auto visitor = []<typename T>(const T &alternative) {
            return ankerl::unordered_dense::hash<T> {}(alternative);
        };
        const auto alternative_hash = std::visit(visitor, obj);

        return logicsim::wyhash_128_bit(index, alternative_hash);
    }
};

#endif
