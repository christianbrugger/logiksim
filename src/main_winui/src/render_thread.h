#pragma once

#include "main_winui/src/render_buffer.h"

#include <gsl/gsl>

#include <windows.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.UI.Input.h>

#include <memory>
#include <optional>
#include <thread>
#include <variant>

namespace logicsim {

//
// Actions
//

/**
 * @brief: Communication from render thread to UI.
 *
 * Class-invariants:
 *   + All methods can be safely called from non-UI threads.
 *   + Interface only type, all functions are pure virtual (C.121)
 */
class IRenderGuiActions {
   public:
    explicit IRenderGuiActions() = default;
    // polymorphic types require virtual destructor (C.127)
    virtual ~IRenderGuiActions() = default;
    // delete copy & move to prevent slicing on polymorphic classes (C.67)
    IRenderGuiActions(const IRenderGuiActions&) = delete;
    IRenderGuiActions(IRenderGuiActions&&) = delete;
    auto operator=(const IRenderGuiActions&) -> IRenderGuiActions& = delete;
    auto operator=(IRenderGuiActions&&) -> IRenderGuiActions& = delete;

   public:
    virtual auto register_swap_chain(
        winrt::Microsoft::Graphics::Canvas::CanvasSwapChain swap_chain) const -> void = 0;
};

//
// Thread
//

[[nodiscard]] auto create_render_thread(std::unique_ptr<IRenderGuiActions> actions,
                                        RenderBufferSink render_sink) -> std::jthread;

}  // namespace logicsim