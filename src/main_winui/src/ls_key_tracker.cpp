#include "pch.h"

#include "main_winui/src/ls_key_tracker.h"

#include "main_winui/src/backend_thread.h"
#include "main_winui/src/ls_xaml_utils.h"

#include <gsl/gsl>

#include <iostream>

namespace logicsim {

SingleKeyTracker::SingleKeyTracker(MouseButton filter, GenerateDoubleClick double_click)
    : filter_ {filter}, generate_double_click_ {double_click} {}

namespace {

/**
 * @brief: Time relative to the system boot time.
 */
[[nodiscard]] auto get_timestamp(const PointerEventData& data)
    -> std::chrono::microseconds {
    return std::chrono::microseconds {data.point.Timestamp()};
}

[[nodiscard]] auto check_double_click(
    GenerateDoubleClick generate_double_click,
    const std::optional<std::chrono::microseconds>& last_press_timestamp,
    const PointerEventData& data) -> bool {
    if (generate_double_click != GenerateDoubleClick::Yes) {
        return false;
    }

    if (!last_press_timestamp.has_value()) {
        return false;
    }

    const auto delta = get_timestamp(data) - last_press_timestamp.value();
    return delta < get_double_click_time_setting();
}

auto generate_press_event(exporting::MouseButton button, const PointerEventData& data,
                          bool is_double_click, BackendTaskSource& tasks) -> void {
    using namespace exporting;

    const auto position = data.point.Position();

    tasks.push(MousePressEvent {
        .position = to_device_position(data.point),
        .modifiers = to_keyboard_modifiers(data.modifiers),
        .button = button,
        .double_click = is_double_click,
    });
}

auto generate_move_event(const PointerEventData& data, BackendTaskSource& tasks) -> void {
    using namespace exporting;

    const auto position = data.point.Position();

    tasks.push(MouseMoveEvent {
        .position = to_device_position(data.point),
        .buttons = to_mouse_buttons(data.point),
    });
}

auto generate_release_event(exporting::MouseButton button, const PointerEventData& data,
                            BackendTaskSource& tasks) -> void {
    using namespace exporting;

    const auto position = data.point.Position();

    tasks.push(MouseReleaseEvent {
        .position = to_device_position(data.point),
        .button = button,
    });
}

}  // namespace

auto SingleKeyTracker::submit_event(const PointerEventData& data,
                                    BackendTaskSource& tasks) -> bool {
    const auto is_pressed_now = is_button_pressed(filter_, data.point);
    auto gen_move_event = false;

    switch (state_) {
        using enum ButtonState;

        case Unpressed: {
            if (is_pressed_now) {
                const auto double_click = check_double_click(generate_double_click_,
                                                             last_press_timestamp_, data);

                generate_press_event(filter_, data, double_click, tasks);

                last_press_timestamp_ = double_click  //
                                            ? std::nullopt
                                            : std::make_optional(get_timestamp(data));
                state_ = Pressed;
                break;
            }
            break;
        }

        case Pressed: {
            if (!is_pressed_now) {
                generate_release_event(filter_, data, tasks);
                state_ = Unpressed;
                break;
            }
            gen_move_event = true;
            break;
        }
    };

    Ensures(is_pressed_now ? state_ == ButtonState::Pressed
                           : state_ == ButtonState::Unpressed);
    return gen_move_event;
}

auto KeyTracker::submit_event(const PointerEventData& data, BackendTaskSource& tasks)
    -> void {
    const auto position = to_device_position(data.point);

    auto gen_move_event = false;
    gen_move_event |= mouse_left_.submit_event(data, tasks);
    gen_move_event |= mouse_right_.submit_event(data, tasks);
    gen_move_event |= mouse_middle_.submit_event(data, tasks);

    // one move events for all buttons
    if (gen_move_event && position != last_position_) {
        generate_move_event(data, tasks);
    }
    last_position_ = position;
}

}  // namespace logicsim