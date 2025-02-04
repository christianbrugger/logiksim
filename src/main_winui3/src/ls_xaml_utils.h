#pragma once

#include <windows.h>
#include <winrt/Microsoft.Graphics.Canvas.h>

namespace logicsim {

// defined by microsoft
constexpr static auto LS_IDENTITY_DPI = 96.0;

[[nodiscard]] auto DeviceLostException_IsDeviceLost(HRESULT hresult) -> bool;

[[nodiscard]] auto is_swap_chain_alive(
    winrt::Microsoft::Graphics::Canvas::CanvasSwapChain const& swap_chain) -> bool;

}  // namespace logicsim
