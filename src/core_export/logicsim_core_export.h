#ifndef LOGICSIM_CORE_LOGICSIM_EXPORT_H
#define LOGICSIM_CORE_LOGICSIM_EXPORT_H

#ifdef __cplusplus
#include <array>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>  // unique_ptr
#include <optional>
#include <stdexcept>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#ifdef _WIN32

#if defined(LS_CORE_LIB_BUILD_SHARED)
#define LS_CORE_API __declspec(dllexport)
#elif defined(LS_CORE_LIB_BUILD_STATIC)
#define LS_CORE_API
#else
#define LS_CORE_API __declspec(dllimport)
#endif

#else  // !_Win32

#if defined(LS_CORE_LIB_BUILD_SHARED) && (defined(__GNUC__) || defined(__clang__))
#define LS_CORE_API __attribute__((visibility("default")))
#else
#define LS_CORE_API
#endif

#endif

#ifdef __cplusplus
#define LS_NOEXCEPT noexcept
#else
#define LS_NOEXCEPT
#endif

#ifdef __cplusplus
#define LS_NODISCARD [[nodiscard]]
#else
#define LS_NODISCARD
#endif

//
// C DLL interface - hourclass pattern
//

#ifdef __cplusplus
extern "C" {
#endif
// NOLINTBEGIN(modernize-use-using)
// NOLINTBEGIN(modernize-use-trailing-return-type)

typedef struct ls_optional_double_t {
    double value;
    bool is_valid;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_optional_double_t&) const -> bool = default;
#endif
} ls_optional_double_t;

typedef struct ls_ui_status_t {
    bool repaint_required;
    bool config_changed;
    bool history_changed;
    bool dialogs_changed;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_ui_status_t&) const -> bool = default;
#endif
} ls_ui_status_t;

typedef struct ls_circuit_t ls_circuit_t;

// circuit::construct
LS_NODISCARD LS_CORE_API ls_circuit_t* ls_circuit_construct() LS_NOEXCEPT;

// circuit::destruct
LS_CORE_API void ls_circuit_destruct(ls_circuit_t* obj) LS_NOEXCEPT;

typedef struct ls_simulation_config_t {
    int64_t simulation_time_rate_ns;
    bool use_wire_delay;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_simulation_config_t&) const -> bool = default;
#endif
} ls_simulation_config_t;

typedef struct ls_render_config_t {
    uint8_t thread_count_enum;
    uint8_t wire_render_style_enum;
    //
    bool do_benchmark;
    bool show_circuit;
    bool show_collision_index;
    bool show_connection_index;
    bool show_selection_index;
    //
    bool show_render_borders;
    bool show_mouse_position;
    bool direct_rendering;
    bool jit_rendering;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_render_config_t&) const -> bool = default;
#endif
} ls_render_config_t;

typedef struct ls_circuit_state_t {
    uint8_t type_enum;
    uint8_t editing_default_mouse_action_enum;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_circuit_state_t&) const -> bool = default;
#endif
} ls_circuit_state_t;

typedef struct ls_ui_config_t {
    ls_simulation_config_t simulation;
    ls_render_config_t render;
    ls_circuit_state_t state;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_ui_config_t&) const -> bool = default;
#endif
} ls_ui_config_t;

// circuit::config
LS_NODISCARD LS_CORE_API ls_ui_config_t ls_circuit_config(const ls_circuit_t* obj)
    LS_NOEXCEPT;

// circuit::set_config
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_set_config(ls_circuit_t* obj, const ls_ui_config_t* config) LS_NOEXCEPT;

typedef struct ls_ui_statistics_t {
    ls_optional_double_t simulation_events_per_second;
    double frames_per_second;
    double pixel_scale;
    int32_t image_width_px;
    int32_t image_height_px;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_ui_statistics_t&) const -> bool = default;
#endif
} ls_ui_statistics_t;

// circuit::statistic
LS_NODISCARD LS_CORE_API ls_ui_statistics_t ls_circuit_statistic(const ls_circuit_t* obj)
    LS_NOEXCEPT;

// circuit::do_action
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_do_action(ls_circuit_t* obj, uint8_t action_enum) LS_NOEXCEPT;

// circuit::load
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_load(ls_circuit_t* obj, uint8_t example_circuit_enum) LS_NOEXCEPT;

/**
 * @brief: Render the layout to the given buffer.
 *
 * Terminates, if either width or height is negative.
 */
LS_CORE_API void ls_circuit_render_layout(ls_circuit_t* obj, int32_t width,
                                          int32_t height, double pixel_ratio,
                                          void* pixel_data, intptr_t stride) LS_NOEXCEPT;

typedef struct ls_point_device_fine_t {
    double x;
    double y;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_point_device_fine_t&) const -> bool = default;
#endif
} ls_point_device_fine_t;

typedef struct ls_angle_delta_t {
    float horizontal_notches;
    float vertical_notches;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_angle_delta_t&) const -> bool = default;
#endif
} ls_angle_delta_t;

typedef struct {
    ls_point_device_fine_t position;
    uint32_t keyboard_modifiers_bitset;
    uint8_t button_enum;
    bool double_click;
} ls_mouse_press_event_t;

// circuit::mouse_press
LS_NODISCARD LS_CORE_API ls_ui_status_t ls_circuit_mouse_press(
    ls_circuit_t* obj, const ls_mouse_press_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    uint32_t buttons_bitset;
} ls_mouse_move_event_t;

// circuit::mouse_move
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_mouse_move(ls_circuit_t* obj, const ls_mouse_move_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    uint8_t button_enum;
} ls_mouse_release_event_t;

// circuit::mouse_release
LS_NODISCARD LS_CORE_API ls_ui_status_t ls_circuit_mouse_release(
    ls_circuit_t* obj, const ls_mouse_release_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    ls_angle_delta_t angle_delta;
    uint32_t keyboard_modifiers_bitset;
} ls_mouse_wheel_event_t;

// circuit::mouse_wheel
LS_NODISCARD LS_CORE_API ls_ui_status_t ls_circuit_mouse_wheel(
    ls_circuit_t* obj, const ls_mouse_wheel_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_mouse_wheel_event_t value;
    bool is_valid;
} ls_combine_wheel_event_result_t;

// combine_wheel_event
LS_NODISCARD LS_CORE_API ls_combine_wheel_event_result_t
ls_combine_wheel_event(const ls_mouse_wheel_event_t* first,
                       const ls_mouse_wheel_event_t* second) LS_NOEXCEPT;

LS_NODISCARD LS_CORE_API ls_ui_status_t ls_circuit_key_press(ls_circuit_t* obj,
                                                             int32_t key) LS_NOEXCEPT;

// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-use-using)
#ifdef __cplusplus
}
#endif

//
// C++ abstraction - header only
//

#ifdef __cplusplus

[[nodiscard]] inline auto operator|(ls_ui_status_t a,
                                    ls_ui_status_t b) -> ls_ui_status_t {
    return ls_ui_status_t {
        .repaint_required = a.repaint_required || b.repaint_required,
        .config_changed = a.config_changed || b.config_changed,
        .history_changed = a.history_changed || b.history_changed,
        .dialogs_changed = a.dialogs_changed || b.dialogs_changed,
    };
};

inline auto operator|=(ls_ui_status_t& a, ls_ui_status_t b) -> ls_ui_status_t& {
    a = a | b;
    return a;
};

namespace logicsim::exporting {

namespace detail {

inline auto ls_expects(auto&& value) -> void {
    if (!bool {value}) {
        std::terminate();
    }
}

// backport from c++23
template <class Enum>
constexpr auto to_underlying(Enum e) noexcept -> std::underlying_type_t<Enum> {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

}  // namespace detail

enum class ThreadCount : uint8_t {
    synchronous = 0,
    two = 1,
    four = 2,
    eight = 3,
};

enum class WireRenderStyle : uint8_t {
    red = 0,
    bold = 1,
    bold_red = 2,
};

enum class CircuitStateType : uint8_t {
    NonInteractive = 0,
    Simulation = 1,
    Editing = 2,
};

enum class DefaultMouseAction : uint8_t {
    // other
    selection = 0,
    insert_wire = 1,

    // logic items
    insert_button = 2,
    insert_led = 3,
    insert_display_number = 4,
    insert_display_ascii = 5,

    insert_and_element = 6,
    insert_or_element = 7,
    insert_xor_element = 8,
    insert_nand_element = 9,
    insert_nor_element = 10,

    insert_buffer_element = 11,
    insert_inverter_element = 12,
    insert_flipflop_jk = 13,
    insert_latch_d = 14,
    insert_flipflop_d = 15,
    insert_flipflop_ms_d = 16,

    insert_clock_generator = 17,
    insert_shift_register = 18,

    // decorations
    insert_decoration_text_element = 19,
};

struct time_rate_t {
    using rep = int64_t;
    using period = std::nano;
    using value_type = std::chrono::duration<rep, period>;

    value_type rate_per_second;

    [[nodiscard]] auto operator==(const time_rate_t& other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_rate_t& other) const = default;
};

struct SimulationConfig {
    time_rate_t simulation_time_rate;
    bool use_wire_delay;

    [[nodiscard]] auto operator==(const SimulationConfig& other) const -> bool = default;
};

struct WidgetRenderConfig {
    ThreadCount thread_count;
    WireRenderStyle wire_render_style;

    bool do_benchmark;
    bool show_circuit;
    bool show_collision_index;
    bool show_connection_index;
    bool show_selection_index;

    bool show_render_borders;
    bool show_mouse_position;
    bool direct_rendering;
    bool jit_rendering;

    [[nodiscard]] auto operator==(const WidgetRenderConfig&) const -> bool = default;
};

struct CircuitWidgetState {
    CircuitStateType type;
    DefaultMouseAction editing_default_mouse_action;

    [[nodiscard]] auto operator==(const CircuitWidgetState&) const -> bool = default;
};

struct CircuitUIConfig {
    SimulationConfig simulation;
    WidgetRenderConfig render;
    CircuitWidgetState state;

    [[nodiscard]] auto operator==(const CircuitUIConfig&) const -> bool = default;
};

enum class UserAction : uint8_t {
    clear_circuit = 0,
    reload_circuit = 1,

    undo = 2,
    redo = 3,
    select_all = 4,
    copy_selected = 5,
    paste_from_clipboard = 6,
    cut_selected = 7,
    delete_selected = 8,

    zoom_in = 9,
    zoom_out = 10,
    reset_view = 11,
};

enum class ExampleCircuitType : uint8_t {
    example_circuit_1 = 1,
    example_circuit_2 = 2,
    example_circuit_3 = 3,
    example_circuit_4 = 4,
};

enum class VirtualKey : uint8_t {
    Enter = 0,
    Escape = 1,
};

enum class MouseButton : uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
};

constexpr inline auto all_mouse_buttons = std::array {
    MouseButton::Left,
    MouseButton::Right,
    MouseButton::Middle,
};

enum class KeyboardModifier : uint8_t {
    Shift = 0,
    Control = 1,
    Alt = 2,
};

constexpr inline auto all_keyboard_modifiers = std::array {
    KeyboardModifier::Shift,
    KeyboardModifier::Control,
    KeyboardModifier::Alt,
};

class MouseButtons {
   public:
    MouseButtons() = default;

    explicit MouseButtons(uint32_t value) : value_ {value} {}

    auto set(MouseButton button, bool value = true) -> MouseButtons& {
        value_.set(detail::to_underlying(button), value);
        return *this;
    }

    [[nodiscard]] auto is_set(MouseButton button) const -> bool {
        return value_.test(detail::to_underlying(button));
    }

    [[nodiscard]] auto value() const -> uint32_t {
        const auto result = value_.to_ulong();

        if (result > std::numeric_limits<uint32_t>::max()) {
            throw std::overflow_error {"Value out of range"};
        }

        return static_cast<uint32_t>(result);
    }

    [[nodiscard]] auto operator==(const MouseButtons&) const -> bool = default;

    [[nodiscard]] explicit operator bool() const {
        return value_.any();
    }

   private:
    std::bitset<all_mouse_buttons.size()> value_;
};

class KeyboardModifiers {
   public:
    KeyboardModifiers() = default;

    explicit KeyboardModifiers(uint32_t value) : value_ {value} {}

    auto set(KeyboardModifier modifier, bool value = true) -> KeyboardModifiers& {
        value_.set(detail::to_underlying(modifier), value);
        return *this;
    }

    [[nodiscard]] auto is_set(KeyboardModifier modifier) const -> bool {
        return value_.test(detail::to_underlying(modifier));
    }

    [[nodiscard]] auto value() const -> uint32_t {
        const auto result = value_.to_ulong();

        if (result > std::numeric_limits<uint32_t>::max()) {
            throw std::overflow_error {"Value out of range"};
        }

        return static_cast<uint32_t>(result);
    }

    [[nodiscard]] auto operator==(const KeyboardModifiers&) const -> bool = default;

    [[nodiscard]] explicit operator bool() const {
        return value_.any();
    }

   private:
    std::bitset<all_keyboard_modifiers.size()> value_;
};

struct MousePressEvent {
    ls_point_device_fine_t position {};
    KeyboardModifiers modifiers {};
    MouseButton button {MouseButton::Left};
    bool double_click {false};

    [[nodiscard]] auto operator==(const MousePressEvent&) const -> bool = default;
};

struct MouseMoveEvent {
    ls_point_device_fine_t position {};
    MouseButtons buttons {};

    [[nodiscard]] auto operator==(const MouseMoveEvent&) const -> bool = default;
};

struct MouseReleaseEvent {
    ls_point_device_fine_t position {};
    MouseButton button {MouseButton::Left};

    [[nodiscard]] auto operator==(const MouseReleaseEvent&) const -> bool = default;
};

struct MouseWheelEvent {
    ls_point_device_fine_t position {};
    ls_angle_delta_t angle_delta {};
    KeyboardModifiers modifiers {};

    [[nodiscard]] auto operator==(const MouseWheelEvent&) const -> bool = default;
};

[[nodiscard]] inline auto combine_wheel_event(const MouseWheelEvent& first,
                                              const MouseWheelEvent& second)
    -> std::optional<MouseWheelEvent>;

namespace detail {

struct LSCircuitDeleter {
    auto operator()(ls_circuit_t* obj) noexcept -> void {
        ls_circuit_destruct(obj);
    };
};

}  // namespace detail

class CircuitInterface {
   public:
    [[nodiscard]] inline auto set_config(const CircuitUIConfig& config) -> ls_ui_status_t;
    [[nodiscard]] inline auto config() const -> CircuitUIConfig;
    [[nodiscard]] inline auto statistics() const -> ls_ui_statistics_t;

    [[nodiscard]] inline auto do_action(UserAction action) -> ls_ui_status_t;
    [[nodiscard]] inline auto load(ExampleCircuitType type) -> ls_ui_status_t;

    inline auto render_layout(int32_t width, int32_t height, double pixel_ratio,
                              void* pixel_data, intptr_t stride) -> void;

    [[nodiscard]] inline auto mouse_press(const MousePressEvent& event) -> ls_ui_status_t;
    [[nodiscard]] inline auto mouse_move(const MouseMoveEvent& event) -> ls_ui_status_t;
    [[nodiscard]] inline auto mouse_release(const MouseReleaseEvent& event)
        -> ls_ui_status_t;
    [[nodiscard]] inline auto mouse_wheel(const MouseWheelEvent& event) -> ls_ui_status_t;
    [[nodiscard]] inline auto key_press(VirtualKey key) -> ls_ui_status_t;

   private:
    [[nodiscard]] inline auto get() const -> const ls_circuit_t*;
    [[nodiscard]] inline auto get() -> ls_circuit_t*;

   private:
    std::unique_ptr<ls_circuit_t, detail::LSCircuitDeleter> obj_ {ls_circuit_construct()};
};

//
// C++ abstraction - Implementation
//

namespace detail {

[[nodiscard]] inline auto from_exp(const MouseWheelEvent& event)
    -> ls_mouse_wheel_event_t {
    return ls_mouse_wheel_event_t {
        .position = event.position,
        .angle_delta = event.angle_delta,
        .keyboard_modifiers_bitset = event.modifiers.value(),
    };
}

[[nodiscard]] inline auto to_exp(const ls_mouse_wheel_event_t& event) -> MouseWheelEvent {
    return MouseWheelEvent {
        .position = event.position,
        .angle_delta = event.angle_delta,
        .modifiers = KeyboardModifiers {event.keyboard_modifiers_bitset},
    };
}

[[nodiscard]] inline auto from_exp(const CircuitUIConfig& config) -> ls_ui_config_t {
    return ls_ui_config_t {
        .simulation =
            ls_simulation_config_t {
                .simulation_time_rate_ns =
                    config.simulation.simulation_time_rate.rate_per_second.count(),
                .use_wire_delay = config.simulation.use_wire_delay,
            },
        .render =
            ls_render_config_t {
                .thread_count_enum = to_underlying(config.render.thread_count),
                .wire_render_style_enum = to_underlying(config.render.wire_render_style),

                .do_benchmark = config.render.do_benchmark,
                .show_circuit = config.render.show_circuit,
                .show_collision_index = config.render.show_collision_index,
                .show_connection_index = config.render.show_connection_index,
                .show_selection_index = config.render.show_selection_index,

                .show_render_borders = config.render.show_render_borders,
                .show_mouse_position = config.render.show_mouse_position,
                .direct_rendering = config.render.direct_rendering,
                .jit_rendering = config.render.jit_rendering,
            },
        .state =
            ls_circuit_state_t {
                .type_enum = to_underlying(config.state.type),
                .editing_default_mouse_action_enum =
                    to_underlying(config.state.editing_default_mouse_action),
            },
    };
}

[[nodiscard]] inline auto to_exp(const ls_ui_config_t& config) -> CircuitUIConfig {
    return CircuitUIConfig {
        .simulation =
            SimulationConfig {
                .simulation_time_rate = time_rate_t {time_rate_t::value_type {
                    config.simulation.simulation_time_rate_ns}},
                .use_wire_delay = config.simulation.use_wire_delay,
            },
        .render =
            WidgetRenderConfig {
                .thread_count = static_cast<ThreadCount>(config.render.thread_count_enum),
                .wire_render_style =
                    static_cast<WireRenderStyle>(config.render.wire_render_style_enum),

                .do_benchmark = config.render.do_benchmark,
                .show_circuit = config.render.show_circuit,
                .show_collision_index = config.render.show_collision_index,
                .show_connection_index = config.render.show_connection_index,
                .show_selection_index = config.render.show_selection_index,

                .show_render_borders = config.render.show_render_borders,
                .show_mouse_position = config.render.show_mouse_position,
                .direct_rendering = config.render.direct_rendering,
                .jit_rendering = config.render.jit_rendering,
            },
        .state =
            CircuitWidgetState {
                .type = static_cast<CircuitStateType>(config.state.type_enum),
                .editing_default_mouse_action = static_cast<DefaultMouseAction>(
                    config.state.editing_default_mouse_action_enum),
            },
    };
}

}  // namespace detail

auto combine_wheel_event(const MouseWheelEvent& first, const MouseWheelEvent& second)
    -> std::optional<MouseWheelEvent> {
    const auto first_c = detail::from_exp(first);
    const auto second_c = detail::from_exp(second);

    if (const auto result = ls_combine_wheel_event(&first_c, &second_c);
        result.is_valid) {
        return detail::to_exp(result.value);
    }
    return std::nullopt;
}

auto CircuitInterface::get() const -> const ls_circuit_t* {
    return obj_.get();
}

auto CircuitInterface::get() -> ls_circuit_t* {
    return obj_.get();
}

auto CircuitInterface::set_config(const CircuitUIConfig& config) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto config_c = detail::from_exp(config);
    return ls_circuit_set_config(get(), &config_c);
}

auto CircuitInterface::config() const -> CircuitUIConfig {
    detail::ls_expects(obj_);

    return detail::to_exp(ls_circuit_config(get()));
}

auto CircuitInterface::statistics() const -> ls_ui_statistics_t {
    detail::ls_expects(obj_);

    return ls_circuit_statistic(get());
}

auto CircuitInterface::do_action(UserAction action) -> ls_ui_status_t {
    detail::ls_expects(obj_);
    return ls_circuit_do_action(get(), detail::to_underlying(action));
}

auto CircuitInterface::load(ExampleCircuitType type) -> ls_ui_status_t {
    detail::ls_expects(obj_);
    return ls_circuit_load(get(), detail::to_underlying(type));
};

auto CircuitInterface::render_layout(int32_t width, int32_t height, double pixel_ratio,
                                     void* pixel_data, intptr_t stride) -> void {
    detail::ls_expects(obj_);
    ls_circuit_render_layout(get(), width, height, pixel_ratio, pixel_data, stride);
}

auto CircuitInterface::mouse_press(const MousePressEvent& event) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_press_event_t {
        .position = event.position,
        .keyboard_modifiers_bitset = event.modifiers.value(),
        .button_enum = detail::to_underlying(event.button),
        .double_click = event.double_click,
    };
    return ls_circuit_mouse_press(get(), &event_c);
};

auto CircuitInterface::mouse_move(const MouseMoveEvent& event) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_move_event_t {
        .position = event.position,
        .buttons_bitset = event.buttons.value(),
    };
    return ls_circuit_mouse_move(get(), &event_c);
};

auto CircuitInterface::mouse_release(const MouseReleaseEvent& event) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_release_event_t {
        .position = event.position,
        .button_enum = detail::to_underlying(event.button),
    };
    return ls_circuit_mouse_release(get(), &event_c);
};

auto CircuitInterface::mouse_wheel(const MouseWheelEvent& event) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto event_c = detail::from_exp(event);
    return ls_circuit_mouse_wheel(get(), &event_c);
};

auto CircuitInterface::key_press(VirtualKey key) -> ls_ui_status_t {
    detail::ls_expects(obj_);
    return ls_circuit_key_press(get(), detail::to_underlying(key));
};

}  // namespace logicsim::exporting

#endif

#endif
