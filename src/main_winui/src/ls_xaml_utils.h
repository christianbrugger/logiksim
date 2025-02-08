#pragma once

#include "core_export/logicsim_core_export.h"

#include <windows.h>
#include <winrt/Microsoft.Graphics.Canvas.h>

#include <chrono>
#include <optional>

namespace logicsim {

// defined by microsoft
constexpr static auto LS_IDENTITY_DPI = 96.0;

[[nodiscard]] auto DeviceLostException_IsDeviceLost(HRESULT hresult) -> bool;

[[nodiscard]] auto is_swap_chain_alive(
    winrt::Microsoft::Graphics::Canvas::CanvasSwapChain const& swap_chain) -> bool;

[[nodiscard]] auto get_double_click_time_setting() -> std::chrono::milliseconds;

[[nodiscard]] auto is_button_pressed(
    exporting::MouseButton button, const winrt::Microsoft::UI::Input::PointerPoint& point)
    -> bool;

[[nodiscard]] auto to_mouse_button(const winrt::Microsoft::UI::Input::PointerPoint& point)
    -> std::optional<logicsim::exporting::MouseButton>;

[[nodiscard]] auto to_mouse_buttons(
    const winrt::Microsoft::UI::Input::PointerPoint& point)
    -> logicsim::exporting::MouseButtons;

[[nodiscard]] auto to_device_position(
    const winrt::Microsoft::UI::Input::PointerPoint& point) -> ls_point_device_fine_t;

[[nodiscard]] auto to_keyboard_modifiers(
    winrt::Windows::System::VirtualKeyModifiers modifiers)
    -> logicsim::exporting::KeyboardModifiers;

[[nodiscard]] auto to_angle_delta(const winrt::Microsoft::UI::Input::PointerPoint& point)
    -> ls_angle_delta_t;

}  // namespace logicsim
