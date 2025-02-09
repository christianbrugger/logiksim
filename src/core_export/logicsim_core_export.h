#ifndef LOGICSIM_CORE_LOGICSIM_EXPORT_H
#define LOGICSIM_CORE_LOGICSIM_EXPORT_H

#ifdef __cplusplus
#include <array>
#include <bitset>
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

    bool do_benchmark;
    bool show_circuit;
    bool show_collision_index;
    bool show_connection_index;
    bool show_selection_index;

    bool show_render_borders;
    bool show_mouse_position;
    bool direct_rendering;
    bool jit_rendering;

#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_render_config_t&) const -> bool = default;
#endif
} ls_render_config_t;

typedef struct ls_circuit_state_t {
    uint8_t state_enum;
    uint8_t editing_default_mouse_action_enum;
    //
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
LS_NODISCARD LS_CORE_API ls_ui_config_t ls_circuit_config(ls_circuit_t* obj) LS_NOEXCEPT;

// circuit::set_config
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_set_config(ls_circuit_t* obj, const ls_ui_config_t* config) LS_NOEXCEPT;

// circuit::load
LS_NODISCARD LS_CORE_API ls_ui_status_t
ls_circuit_load(ls_circuit_t* obj, int32_t example_circuit) LS_NOEXCEPT;

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
    int32_t button_enum;
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
    int32_t button_enum;
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
    synchronous,
    two,
    four,
    eight,
};

enum class WireRenderStyle : uint8_t {
    red,
    bold,
    bold_red,
};

enum class DefaultMouseAction : uint8_t {
    // other
    selection,
    insert_wire,

    // logic items
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

    // decorations
    insert_decoration_text_element,
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

auto CircuitInterface::load(ExampleCircuitType type) -> ls_ui_status_t {
    detail::ls_expects(obj_);
    return ls_circuit_load(obj_.get(), static_cast<int32_t>(type));
};

auto CircuitInterface::render_layout(int32_t width, int32_t height, double pixel_ratio,
                                     void* pixel_data, intptr_t stride) -> void {
    detail::ls_expects(obj_);
    ls_circuit_render_layout(obj_.get(), width, height, pixel_ratio, pixel_data, stride);
}

auto CircuitInterface::mouse_press(const MousePressEvent& event) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_press_event_t {
        .position = event.position,
        .keyboard_modifiers_bitset = event.modifiers.value(),
        .button_enum = detail::to_underlying(event.button),
        .double_click = event.double_click,
    };
    return ls_circuit_mouse_press(obj_.get(), &event_c);
};

auto CircuitInterface::mouse_move(const MouseMoveEvent& event) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_move_event_t {
        .position = event.position,
        .buttons_bitset = event.buttons.value(),
    };
    return ls_circuit_mouse_move(obj_.get(), &event_c);
};

auto CircuitInterface::mouse_release(const MouseReleaseEvent& event) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_release_event_t {
        .position = event.position,
        .button_enum = detail::to_underlying(event.button),
    };
    return ls_circuit_mouse_release(obj_.get(), &event_c);
};

auto CircuitInterface::mouse_wheel(const MouseWheelEvent& event) -> ls_ui_status_t {
    detail::ls_expects(obj_);

    const auto event_c = detail::from_exp(event);
    return ls_circuit_mouse_wheel(obj_.get(), &event_c);
};

auto CircuitInterface::key_press(VirtualKey key) -> ls_ui_status_t {
    detail::ls_expects(obj_);
    return ls_circuit_key_press(obj_.get(), detail::to_underlying(key));
};

}  // namespace logicsim::exporting

#endif

#endif
