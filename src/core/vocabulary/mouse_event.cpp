#include "core/vocabulary/mouse_event.h"

namespace logicsim {

template <>
auto format(MouseButtonType type) -> std::string {
    switch (type) {
        using enum MouseButtonType;

        case LeftButton:
            return "LeftButton";
        case RightButton:
            return "RightButton";
        case MiddleButton:
            return "MiddleButton";
    };
    std::terminate();
}

template <>
auto format(MouseEventType type) -> std::string {
    switch (type) {
        using enum MouseEventType;

        case Press:
            return "Press";
        case Move:
            return "Move";
        case Release:
            return "Release";
        case DoubleClick:
            return "DoubleClick";
    };
    std::terminate();
}

template <>
auto format(KeyboardModifierType type) -> std::string {
    switch (type) {
        using enum KeyboardModifierType;

        case NoModifier:
            return "NoModifier";
        case ShiftModifier:
            return "ShiftModifier";
        case ControlModifier:
            return "ControlModifier";
        case AltModifier:
            return "AltModifier";
    };
    std::terminate();
}

auto MouseEvent::format() const -> std::string {
    return fmt::format("MouseEvent(position = {}, button = {}, type = {}, modifier = {})",
                       position, button, type, modifier);
}

}  // namespace logicsim
