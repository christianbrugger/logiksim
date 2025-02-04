#include "pch.h"

#include "main_winui3/src/ls_xaml_utils.h"

// #include <dwmapi.h>

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

}  // namespace logicsim