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

typedef struct ls_circuit_ui_status {
    bool repaint_required;
    bool config_changed;
    bool history_changed;
    bool dialogs_changed;
#ifdef __cplusplus
    [[nodiscard]] auto operator==(const ls_circuit_ui_status&) const -> bool = default;
#endif
} ls_ui_status;

typedef struct ls_circuit_t ls_circuit_t;

LS_NODISCARD LS_CORE_API ls_circuit_t* ls_circuit_construct() LS_NOEXCEPT;

LS_CORE_API void ls_circuit_destruct(ls_circuit_t* obj) LS_NOEXCEPT;

LS_NODISCARD LS_CORE_API ls_ui_status
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
    uint32_t keyboard_modifiers;
    int32_t button;
    bool double_click;
} ls_mouse_press_event_t;

LS_NODISCARD LS_CORE_API ls_ui_status ls_circuit_mouse_press(
    ls_circuit_t* obj, const ls_mouse_press_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    uint32_t buttons;
} ls_mouse_move_event_t;

LS_NODISCARD LS_CORE_API ls_ui_status
ls_circuit_mouse_move(ls_circuit_t* obj, const ls_mouse_move_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    int32_t button;
} ls_mouse_release_event_t;

LS_NODISCARD LS_CORE_API ls_ui_status ls_circuit_mouse_release(
    ls_circuit_t* obj, const ls_mouse_release_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_point_device_fine_t position;
    ls_angle_delta_t angle_delta;
    uint32_t keyboard_modifiers;
} ls_mouse_wheel_event_t;

LS_NODISCARD LS_CORE_API ls_ui_status ls_circuit_mouse_wheel(
    ls_circuit_t* obj, const ls_mouse_wheel_event_t* event) LS_NOEXCEPT;

typedef struct {
    ls_mouse_wheel_event_t value;
    bool is_valid;
} ls_combine_wheel_event_result_t;

LS_NODISCARD LS_CORE_API ls_combine_wheel_event_result_t
ls_combine_wheel_event(const ls_mouse_wheel_event_t* first,
                       const ls_mouse_wheel_event_t* second) LS_NOEXCEPT;

LS_NODISCARD LS_CORE_API ls_ui_status ls_circuit_key_press(ls_circuit_t* obj,
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

[[nodiscard]] inline auto operator|(ls_ui_status a, ls_ui_status b) -> ls_ui_status {
    return ls_ui_status {
        .repaint_required = a.repaint_required || b.repaint_required,
        .config_changed = a.config_changed || b.config_changed,
        .history_changed = a.history_changed || b.history_changed,
        .dialogs_changed = a.dialogs_changed || b.dialogs_changed,
    };
};

inline auto operator|=(ls_ui_status& a, ls_ui_status b) -> ls_ui_status& {
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

// C++23 backport
template <class Enum>
constexpr auto to_underlying(Enum e) noexcept -> std::underlying_type_t<Enum> {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

}  // namespace detail

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
    [[nodiscard]] inline auto load(ExampleCircuitType type) -> ls_ui_status;

    inline auto render_layout(int32_t width, int32_t height, double pixel_ratio,
                              void* pixel_data, intptr_t stride) -> void;

    [[nodiscard]] inline auto mouse_press(const MousePressEvent& event) -> ls_ui_status;
    [[nodiscard]] inline auto mouse_move(const MouseMoveEvent& event) -> ls_ui_status;
    [[nodiscard]] inline auto mouse_release(const MouseReleaseEvent& event)
        -> ls_ui_status;
    [[nodiscard]] inline auto mouse_wheel(const MouseWheelEvent& event) -> ls_ui_status;
    [[nodiscard]] inline auto key_press(VirtualKey key) -> ls_ui_status;

   private:
    std::unique_ptr<ls_circuit_t, detail::LSCircuitDeleter> obj_ {ls_circuit_construct()};
};

//
// C++ abstraction - Implementation
//

namespace detail {

[[nodiscard]] inline auto to_c(const MouseWheelEvent& event) -> ls_mouse_wheel_event_t {
    return ls_mouse_wheel_event_t {
        .position = event.position,
        .angle_delta = event.angle_delta,
        .keyboard_modifiers = event.modifiers.value(),
    };
}

[[nodiscard]] inline auto to_cpp(const ls_mouse_wheel_event_t& event) -> MouseWheelEvent {
    return MouseWheelEvent {
        .position = event.position,
        .angle_delta = event.angle_delta,
        .modifiers = KeyboardModifiers {event.keyboard_modifiers},
    };
}

}  // namespace detail

auto combine_wheel_event(const MouseWheelEvent& first, const MouseWheelEvent& second)
    -> std::optional<MouseWheelEvent> {
    const auto first_c = detail::to_c(first);
    const auto second_c = detail::to_c(second);

    if (const auto result = ls_combine_wheel_event(&first_c, &second_c);
        result.is_valid) {
        return detail::to_cpp(result.value);
    }
    return std::nullopt;
}

auto CircuitInterface::load(ExampleCircuitType type) -> ls_ui_status {
    detail::ls_expects(obj_);
    return ls_circuit_load(obj_.get(), static_cast<int32_t>(type));
};

auto CircuitInterface::render_layout(int32_t width, int32_t height, double pixel_ratio,
                                     void* pixel_data, intptr_t stride) -> void {
    detail::ls_expects(obj_);
    ls_circuit_render_layout(obj_.get(), width, height, pixel_ratio, pixel_data, stride);
}

auto CircuitInterface::mouse_press(const MousePressEvent& event) -> ls_ui_status {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_press_event_t {
        .position = event.position,
        .keyboard_modifiers = event.modifiers.value(),
        .button = detail::to_underlying(event.button),
        .double_click = event.double_click,
    };
    return ls_circuit_mouse_press(obj_.get(), &event_c);
};

auto CircuitInterface::mouse_move(const MouseMoveEvent& event) -> ls_ui_status {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_move_event_t {
        .position = event.position,
        .buttons = event.buttons.value(),
    };
    return ls_circuit_mouse_move(obj_.get(), &event_c);
};

auto CircuitInterface::mouse_release(const MouseReleaseEvent& event) -> ls_ui_status {
    detail::ls_expects(obj_);

    const auto event_c = ls_mouse_release_event_t {
        .position = event.position,
        .button = detail::to_underlying(event.button),
    };
    return ls_circuit_mouse_release(obj_.get(), &event_c);
};

auto CircuitInterface::mouse_wheel(const MouseWheelEvent& event) -> ls_ui_status {
    detail::ls_expects(obj_);

    const auto event_c = detail::to_c(event);
    return ls_circuit_mouse_wheel(obj_.get(), &event_c);
};

auto CircuitInterface::key_press(VirtualKey key) -> ls_ui_status {
    detail::ls_expects(obj_);
    return ls_circuit_key_press(obj_.get(), detail::to_underlying(key));
};

}  // namespace logicsim::exporting

#endif

#endif
