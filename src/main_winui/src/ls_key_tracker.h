#pragma once

#include "core_export/logicsim_core_export.h"

#include <optional>

namespace logicsim {

class BackendTaskSource;

enum class GenerateDoubleClick {
    Yes,
    No,
};

namespace key_tracker {

enum class ButtonState {
    Unpressed,
    Pressed,
};

}

struct PointerEventData {
    winrt::Microsoft::UI::Input::PointerPoint point;
    winrt::Windows::System::VirtualKeyModifiers modifiers;
};

/**
 * @brief: Track key events and generate press, move and release events.
 *
 * For the tracker to work correctly it needs to all the following events:
 *   + PointerPressed
 *   + PointerMoved
 *   + PointerReleased
 *   + PointerCanceled
 *   + PointerCaptureLost
 */
class SingleKeyTracker {
   public:
    using ButtonState = key_tracker::ButtonState;
    using MouseButton = exporting::MouseButton;

   public:
    SingleKeyTracker() = default;
    explicit SingleKeyTracker(MouseButton filter, GenerateDoubleClick double_click);

    auto register_event(const PointerEventData& data, BackendTaskSource& tasks) -> bool;

   private:
    MouseButton filter_ {MouseButton::Left};
    GenerateDoubleClick generate_double_click_ {GenerateDoubleClick::No};

    std::optional<std::chrono::microseconds> last_press_timestamp_ {};
    ButtonState state_ {ButtonState::Unpressed};
};

class KeyTracker {
   public:
    using MouseButton = exporting::MouseButton;

   public:
    auto register_event(const PointerEventData& data, BackendTaskSource& tasks) -> void;

   private:
    std::optional<ls_point_device_fine_t> last_position_ {};

    SingleKeyTracker mouse_left_ {MouseButton::Left, GenerateDoubleClick::Yes};
    SingleKeyTracker mouse_right_ {MouseButton::Right, GenerateDoubleClick::No};
    SingleKeyTracker mouse_middle_ {MouseButton::Middle, GenerateDoubleClick::No};
};

}  // namespace logicsim