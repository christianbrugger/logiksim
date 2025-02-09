#ifndef LOGICSIM_CORE_VOCABULARY_MOUSE_EVENT_H
#define LOGICSIM_CORE_VOCABULARY_MOUSE_EVENT_H

#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/vocabulary/point_device_fine.h"

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>

namespace logicsim {

//
// Virtual Key
//

/**
 * @brief: Virtual keys used in the application.
 */
enum class VirtualKey : uint8_t {
    Enter,
    Escape,
};

template <>
[[nodiscard]] auto format(VirtualKey key) -> std::string;

//
// Mouse Button
//

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

template <>
[[nodiscard]] auto format(MouseButton type) -> std::string;

/**
 * @brief: Bit-set of mouse buttons.
 */
class MouseButtons {
   public:
    [[nodiscard]] auto operator==(const MouseButtons&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] explicit operator bool() const;

    auto set(MouseButton button, bool value = true) -> MouseButtons&;
    [[nodiscard]] auto is_set(MouseButton button) const -> bool;

    // matches a single button
    [[nodiscard]] auto operator==(MouseButton button) const -> bool;

   private:
    std::bitset<all_mouse_buttons.size()> value_;
};

//
// Keyboard Modifier
//

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

template <>
[[nodiscard]] auto format(KeyboardModifier type) -> std::string;

/**
 * @brief: Bit-set of keyboard modifiers.
 */
class KeyboardModifiers {
   public:
    [[nodiscard]] auto operator==(const KeyboardModifiers&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] explicit operator bool() const;

    auto set(KeyboardModifier modifier, bool value = true) -> KeyboardModifiers&;
    [[nodiscard]] auto is_set(KeyboardModifier modifier) const -> bool;

    // matches a single modifier
    [[nodiscard]] auto operator==(KeyboardModifier modifier) const -> bool;

   private:
    std::bitset<all_keyboard_modifiers.size()> value_;
};

/**
 * @brief: Angle delta for mouse wheel events.
 *
 * Each increment of 1 is an increment of the wheel.
 */
struct angle_delta_t {
    float horizontal_notches {};  // +1 scroll left,  -1 scroll right
    float vertical_notches {};    // +1 scroll up,    -1 scroll down

    [[nodiscard]] auto operator==(const angle_delta_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] constexpr auto operator+(angle_delta_t lhs,
                                       angle_delta_t rhs) noexcept -> angle_delta_t;

//
// Mouse Events
//

struct MousePressEvent {
    point_device_fine_t position {};
    KeyboardModifiers modifiers {};
    MouseButton button {MouseButton::Left};
    bool double_click {false};

    [[nodiscard]] auto operator==(const MousePressEvent&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct MouseMoveEvent {
    point_device_fine_t position {};
    MouseButtons buttons {};

    [[nodiscard]] auto operator==(const MouseMoveEvent&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct MouseReleaseEvent {
    point_device_fine_t position {};
    MouseButton button {MouseButton::Left};

    [[nodiscard]] auto operator==(const MouseReleaseEvent&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

struct MouseWheelEvent {
    point_device_fine_t position {};
    angle_delta_t angle_delta {};
    KeyboardModifiers modifiers {};

    [[nodiscard]] auto operator==(const MouseWheelEvent&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] auto combine_wheel_event(const MouseWheelEvent& first,
                                       const MouseWheelEvent& second)
    -> std::optional<MouseWheelEvent>;

//
// Implementation
//

constexpr auto operator+(angle_delta_t lhs, angle_delta_t rhs) noexcept -> angle_delta_t {
    return angle_delta_t {
        .horizontal_notches = lhs.horizontal_notches + rhs.horizontal_notches,
        .vertical_notches = lhs.vertical_notches + rhs.vertical_notches,
    };
}

}  // namespace logicsim

#endif
