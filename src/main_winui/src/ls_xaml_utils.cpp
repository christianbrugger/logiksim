#include "pch.h"

#include "main_winui/src/ls_xaml_utils.h"

#include "core_export/logicsim_core_export.h"

#include <winuser.h>

namespace logicsim {

auto DeviceLostException_IsDeviceLost(HRESULT hresult) -> bool {
    // https://microsoft.github.io/Win2D/WinUI2/html/HandlingDeviceLost.htm

    // Copied from (this header is not shipped):
    // DeviceLostException::IsDeviceLostHResult(hresult)
    // https://github.com/microsoft/Win2D/blob/451d68c751b9d783e55a930044603543f656a46f/winrt/inc/ErrorHandling.h#L57

    switch (hresult) {
        case DXGI_ERROR_DEVICE_HUNG:
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_RESET:
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        case DXGI_ERROR_INVALID_CALL:
        case D2DERR_RECREATE_TARGET:
            return true;

        default:
            return false;
    };
}

auto is_swap_chain_alive(
    winrt::Microsoft::Graphics::Canvas::CanvasSwapChain const& swap_chain) -> bool {
    if (!swap_chain) {
        return false;
    }

    auto device = swap_chain.Device();
    return device && !device.IsDeviceLost();
}

auto get_double_click_time_setting() -> std::chrono::milliseconds {
    return std::chrono::milliseconds {GetDoubleClickTime()};
}

auto is_button_pressed(exporting::MouseButton filter,
                       const winrt::Microsoft::UI::Input::PointerPoint& point) -> bool {
    if (point.PointerDeviceType() ==
        winrt::Microsoft::UI::Input::PointerDeviceType::Mouse) {
        const auto properties = point.Properties();

        switch (filter) {
            using enum exporting::MouseButton;

            case Left:
                return properties.IsLeftButtonPressed();
            case Right:
                return properties.IsRightButtonPressed();
            case Middle:
                return properties.IsMiddleButtonPressed();
        };
    }
    return false;
}

auto to_mouse_button(const winrt::Microsoft::UI::Input::PointerPoint& point)
    -> std::optional<logicsim::exporting::MouseButton> {
    using namespace logicsim::exporting;

    if (point.PointerDeviceType() ==
        winrt::Microsoft::UI::Input::PointerDeviceType::Mouse) {
        const auto properties = point.Properties();

        if (properties.IsLeftButtonPressed()) {
            return MouseButton::Left;
        }
        if (properties.IsRightButtonPressed()) {
            return MouseButton::Right;
        }
        if (properties.IsMiddleButtonPressed()) {
            return MouseButton::Middle;
        }
    }
    return std::nullopt;
}

auto to_mouse_buttons(const winrt::Microsoft::UI::Input::PointerPoint& point)
    -> logicsim::exporting::MouseButtons {
    using namespace logicsim::exporting;

    auto buttons = MouseButtons {};

    if (point.PointerDeviceType() ==
        winrt::Microsoft::UI::Input::PointerDeviceType::Mouse) {
        const auto properties = point.Properties();

        if (properties.IsLeftButtonPressed()) {
            buttons.set(MouseButton::Left);
        }
        if (properties.IsRightButtonPressed()) {
            buttons.set(MouseButton::Right);
        }
        if (properties.IsMiddleButtonPressed()) {
            buttons.set(MouseButton::Middle);
        }
    }

    return buttons;
}

auto to_device_position(const winrt::Microsoft::UI::Input::PointerPoint& point)
    -> ls_point_device_fine_t {
    const auto position = point.Position();
    return ls_point_device_fine_t {.x = position.X, .y = position.Y};
}

namespace {

[[nodiscard]] auto is_set(winrt::Windows::System::VirtualKeyModifiers modifiers,
                          winrt::Windows::System::VirtualKeyModifiers query) -> bool {
    using T = std::underlying_type_t<winrt::Windows::System::VirtualKeyModifiers>;

    return (static_cast<T>(modifiers) & static_cast<T>(query)) != T {0};
}

}  // namespace

auto to_keyboard_modifiers(winrt::Windows::System::VirtualKeyModifiers modifiers)
    -> logicsim::exporting::KeyboardModifiers {
    using namespace logicsim::exporting;
    using namespace winrt::Windows::System;

    auto result = KeyboardModifiers {};

    if (is_set(modifiers, VirtualKeyModifiers::Shift)) {
        result.set(KeyboardModifier::Shift);
    }
    if (is_set(modifiers, VirtualKeyModifiers::Control)) {
        result.set(KeyboardModifier::Control);
    }
    if (is_set(modifiers, VirtualKeyModifiers::Menu)) {
        result.set(KeyboardModifier::Alt);
    }

    return result;
}

auto to_angle_delta(const winrt::Microsoft::UI::Input::PointerPoint& point)
    -> ls_angle_delta_t {
    static_assert(WHEEL_DELTA > 0);
    const auto notch = gsl::narrow<float>(point.Properties().MouseWheelDelta()) /
                       gsl::narrow<float>(WHEEL_DELTA);

    if (point.Properties().IsHorizontalMouseWheel()) {
        return ls_angle_delta_t {.horizontal_notch = notch, .vertical_notch = 0};
    }
    return ls_angle_delta_t {.horizontal_notch = 0, .vertical_notch = notch};
}

}  // namespace logicsim