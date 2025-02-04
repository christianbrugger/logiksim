#ifndef LOGICSIM_CORE_VOCABULARY_MOUSE_EVENT_H
#define LOGICSIM_CORE_VOCABULARY_MOUSE_EVENT_H

#include "core/format/enum.h"
#include "core/format/struct.h"
#include "core/vocabulary/point_device_fine.h"

#include <cstdint>

namespace logicsim {

enum class MouseButtonType : int8_t {
    LeftButton,
    RightButton,
    MiddleButton,
};

template <>
[[nodiscard]] auto format(MouseButtonType type) -> std::string;

enum class MouseEventType : int8_t {
    Press,
    Move,
    Release,
    DoubleClick,
};

template <>
[[nodiscard]] auto format(MouseEventType type) -> std::string;

enum class KeyboardModifierType : int8_t {
    NoModifier = 0,
    ShiftModifier = 1,
    ControlModifier = 2,
    AltModifier = 4,
};

template <>
[[nodiscard]] auto format(KeyboardModifierType type) -> std::string;

struct MouseEvent {
    point_device_fine_t position {};
    MouseButtonType button {MouseButtonType::LeftButton};
    MouseEventType type {MouseEventType::Press};
    KeyboardModifierType modifier {KeyboardModifierType::NoModifier};

    [[nodiscard]] auto operator==(const MouseEvent&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

}  // namespace logicsim

#endif
