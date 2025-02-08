#include "core/vocabulary/mouse_event.h"

#include "core/algorithm/fmt_join.h"
#include "core/algorithm/to_underlying.h"

namespace logicsim {

//
// Mouse Button
//

template <>
auto format(MouseButton type) -> std::string {
    switch (type) {
        using enum MouseButton;

        case Left:
            return "Left";
        case Right:
            return "Right";
        case Middle:
            return "Middle";
    };
    std::terminate();
}

auto MouseButtons::format() const -> std::string {
    const auto is_selected = [this](MouseButton button) { return is_set(button); };
    const auto inner =
        fmt_join(", ", all_mouse_buttons | std::ranges::views::filter(is_selected));
    return "[" + inner + "]";
}

MouseButtons::operator bool() const {
    return value_.any();
}

auto MouseButtons::set(MouseButton button, bool value) -> MouseButtons& {
    value_.set(to_underlying(button), value);
    return *this;
}

auto MouseButtons::is_set(MouseButton button) const -> bool {
    return value_.test(to_underlying(button));
}

//
// Keyboard Modifier
//

template <>
auto format(KeyboardModifier type) -> std::string {
    switch (type) {
        using enum KeyboardModifier;

        case Shift:
            return "Shift";
        case Control:
            return "Control";
        case Alt:
            return "Alt";
    };
    std::terminate();
}

auto KeyboardModifiers::format() const -> std::string {
    const auto selected = [this](KeyboardModifier button) { return is_set(button); };
    const auto inner =
        fmt_join(", ", all_keyboard_modifiers | std::ranges::views::filter(selected));
    return "[" + inner + "]";
}

KeyboardModifiers::operator bool() const {
    return value_.any();
}

auto KeyboardModifiers::set(KeyboardModifier modifier, bool value) -> KeyboardModifiers& {
    value_.set(to_underlying(modifier), value);
    return *this;
}

auto KeyboardModifiers::is_set(KeyboardModifier modifier) const -> bool {
    return value_.test(to_underlying(modifier));
}

//
// Angle Delta
//

auto AngleDelta::format() const -> std::string {
    return fmt::format("(horizontal_notch = {}, vertical_notch = {})", horizontal_notch,
                       vertical_notch);
}

//
// Mouse Events
//

auto MousePressEvent::format() const -> std::string {
    return fmt::format(
        "MousePressEvent(position = {}, button = {}, modifiers = {}, double_click = {})",
        position, button, modifiers, double_click);
}

auto MouseMoveEvent::format() const -> std::string {
    return fmt::format("MouseMoveEvent(position = {}, buttons = {})", position, buttons);
}

auto MouseReleaseEvent::format() const -> std::string {
    return fmt::format("MouseReleasesEvent(position = {}, button = {})", position,
                       button);
}

auto MouseWheelEvent::format() const -> std::string {
    return fmt::format("MouseWheelEvent(position = {}, angle_delta = {}, modifiers = {})",
                       position, angle_delta, modifiers);
}

}  // namespace logicsim
