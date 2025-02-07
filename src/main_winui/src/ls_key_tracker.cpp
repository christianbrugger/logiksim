#include "pch.h"

#include "main_winui/src/ls_key_tracker.h"

#include "main_winui/src/backend_thread.h"
#include "main_winui/src/ls_xaml_utils.h"

#include <gsl/gsl>

namespace logicsim {

SingleKeyTracker::SingleKeyTracker(MouseButton filter, GenerateDoubleClick double_click)
    : filter_ {filter}, generate_double_click_ {double_click} {}

namespace {

auto generate_press_event(exporting::MouseButton button,
                          const winrt::Microsoft::UI::Input::PointerPoint& point,
                          winrt::Windows::System::VirtualKeyModifiers modifiers,
                          BackendTaskSource& tasks) -> void {
    using namespace exporting;

    const auto position = point.Position();

    tasks.push(MousePressEvent {
        .position = ls_point_device_fine_t {.x = position.X, .y = position.Y},
        .modifiers = to_keyboard_modifiers(modifiers),
        .button = button,
        .double_click = false,
    });
}

auto generate_move_event(const winrt::Microsoft::UI::Input::PointerPoint& point,
                         BackendTaskSource& tasks) -> void {
    using namespace exporting;

    const auto position = point.Position();

    tasks.push(MouseMoveEvent {
        .position = ls_point_device_fine_t {.x = position.X, .y = position.Y},
        .buttons = to_mouse_buttons(point),
    });
}

auto generate_release_event(exporting::MouseButton button,
                            const winrt::Microsoft::UI::Input::PointerPoint& point,
                            BackendTaskSource& tasks) -> void {
    using namespace exporting;

    const auto position = point.Position();

    tasks.push(MouseReleaseEvent {
        .position = ls_point_device_fine_t {.x = position.X, .y = position.Y},
        .button = button,
    });
}

}  // namespace

auto SingleKeyTracker::register_event(
    const winrt::Microsoft::UI::Input::PointerPoint& point,
    winrt::Windows::System::VirtualKeyModifiers modifiers, BackendTaskSource& tasks)
    -> bool {
    const auto is_pressed_now = is_button_pressed(filter_, point);
    const auto position = to_device_position(point);
    auto gen_move_event = false;

    switch (state_) {
        using enum ButtonState;

        case Unpressed: {
            if (is_pressed_now) {
                generate_press_event(filter_, point, modifiers, tasks);
                state_ = Pressed;
                break;
            }
            break;
        }

        case Pressed: {
            if (!is_pressed_now) {
                generate_release_event(filter_, point, tasks);
                state_ = Unpressed;
                break;
            }
            if (position != last_position_) {
                gen_move_event = true;
            }
            break;
        }
    };

    last_position_ = position;
    Ensures(is_pressed_now ? state_ == ButtonState::Pressed
                           : state_ == ButtonState::Unpressed);
    return gen_move_event;
}

auto KeyTracker::register_event(const winrt::Microsoft::UI::Input::PointerPoint& point,
                                winrt::Windows::System::VirtualKeyModifiers modifiers,
                                BackendTaskSource& tasks) -> void {
    auto gen_move_event = false;

    gen_move_event |= mouse_left_.register_event(point, modifiers, tasks);
    gen_move_event |= mouse_right_.register_event(point, modifiers, tasks);
    gen_move_event |= mouse_middle_.register_event(point, modifiers, tasks);

    if (gen_move_event) {
        generate_move_event(point, tasks);
    }
}

}  // namespace logicsim