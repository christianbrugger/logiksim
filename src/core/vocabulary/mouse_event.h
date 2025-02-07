#ifndef LOGICSIM_CORE_VOCABULARY_MOUSE_EVENT_H
#define LOGICSIM_CORE_VOCABULARY_MOUSE_EVENT_H

#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/vocabulary/point_device_fine.h"

#include <array>
#include <bitset>
#include <cstdint>

namespace logicsim {

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

   private:
    std::bitset<all_keyboard_modifiers.size()> value_;
};

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

}  // namespace logicsim

#endif
