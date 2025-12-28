#include "pch.h"

#include "main_winui/src/render_thread.h"

#include "main_winui/src/ls_event_counter.h"
#include "main_winui/src/ls_xaml_utils.h"

#include <exception>
#include <print>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace logicsim {

//
// Graphics Resources
//

/**
 * @brief: Holds all allocated graphics resources.
 *
 * Class-invariance:
 *   + swap-chain stays the same once the object is created.
 */
class GraphicsResources {
   public:
    // Resources created for this specific swap chain.
    Microsoft::Graphics::Canvas::CanvasBitmap bitmap {nullptr};
    GuiEventCounter fps_counter {};

   public:
    explicit GraphicsResources() = default;
    explicit GraphicsResources(Microsoft::Graphics::Canvas::CanvasSwapChain swap_chain);
    auto SwapChain() const -> Microsoft::Graphics::Canvas::CanvasSwapChain const&;

   private:
    Microsoft::Graphics::Canvas::CanvasSwapChain swap_chain_ {nullptr};
};

GraphicsResources::GraphicsResources(
    Microsoft::Graphics::Canvas::CanvasSwapChain swap_chain)
    : swap_chain_ {std::move(swap_chain)} {}

auto GraphicsResources::SwapChain() const
    -> Microsoft::Graphics::Canvas::CanvasSwapChain const& {
    return swap_chain_;
}

namespace {

/**
 * Throws hresult_error, if device is lost.
 * Throws hresult_error, if args are invalid (sizes too big) of E_INVALIDARG.
 */
auto ensure_swap_chain_initialized(GraphicsResources& resources,
                                   const SwapChainParams& params,
                                   const IRenderGuiActions& actions) -> void {
    using namespace winrt::Microsoft::Graphics::Canvas;

    const auto dpi = params.dpi();
    const auto size =
        Windows::Foundation::Size {params.width_device(), params.height_device()};

    if (size.Width == 0 || size.Height == 0) {
        resources = GraphicsResources {};
        return;
    }

    if (is_swap_chain_alive(resources.SwapChain())) {
        if (resources.SwapChain().Size() != size || resources.SwapChain().Dpi() != dpi) {
            resources.SwapChain().ResizeBuffers(size.Width, size.Height, dpi);
        }
        return;
    }

    if (const auto device = CanvasDevice::GetSharedDevice()) {
        resources = GraphicsResources {
            CanvasSwapChain {device, size.Width, size.Height, dpi, LS_CANVAS_PIXEL_FORMAT,
                             LS_CANVAS_BUFFER_COUNT, LS_CANVAS_ALPHA_MODE}};
        actions.register_swap_chain(resources.SwapChain());
    }
}

/**
 * Throws hresult_error, if device is lost.
 * Throws hresult_error, if args are invalid (sizes too big) of E_INVALIDARG.
 */
auto update_bitmap(GraphicsResources& resources, const SwapChainParams& params,
                   const frame_t& frame) -> BufferDrawStatus {
    Expects(frame_buffer_size(params) == frame.size());

    const auto size_pixel = Windows::Graphics::Imaging::BitmapSize {
        .Width = gsl::narrow<uint32_t>(params.width_pixel()),
        .Height = gsl::narrow<uint32_t>(params.height_pixel()),
    };

    if (frame.size() == 0) {
        // return true for zero size frames even if there is no swap-chain,
        // so the frame is consumed
        return BufferDrawStatus::DrawingSucceded;
    }
    if (!resources.SwapChain()) {
        return BufferDrawStatus::DrawingFailed;
    }

    if (resources.bitmap && resources.bitmap.SizeInPixels() == size_pixel) {
        resources.bitmap.SetPixelBytes(frame);
        return BufferDrawStatus::DrawingSucceded;
    }

    resources.bitmap = winrt::Microsoft::Graphics::Canvas::CanvasBitmap::CreateFromBytes(
        resources.SwapChain(), frame, params.width_pixel(), params.height_pixel(),
        LS_CANVAS_PIXEL_FORMAT, params.dpi(), LS_CANVAS_ALPHA_MODE);
    Expects(resources.bitmap);
    Expects(resources.bitmap.SizeInPixels() == size_pixel);

    return BufferDrawStatus::DrawingSucceded;
}

/**
 * Throws hresult_error, if device is lost.
 */
auto render_frame(GraphicsResources& resources) -> void {
    if (!resources.SwapChain()) {
        return;
    }
    const auto session =
        resources.SwapChain().CreateDrawingSession(winrt::Microsoft::UI::Colors::Gray());
    if (!session) {
        return;
    }

    const auto fps = resources.fps_counter.events_per_second();
    resources.fps_counter.count_event();

    if (resources.bitmap) {
        session.DrawImage(resources.bitmap);
    }

    session.FillRectangle(5, 5, 220, 40, winrt::Microsoft::UI::Colors::Gray());
    const auto fps_str = std::format(L"{:>8.2f} fps, {:>7.2f} ms", fps, 1000 / fps);
    session.DrawText(fps_str, 5, 10, winrt::Microsoft::UI::Colors::Black());
}

/**
 * Throws hresult_error, if device is lost.
 */
auto present_frame(GraphicsResources& resources) -> void {
    if (!resources.SwapChain()) {
        return;
    }

    // 0 = don't wait for v-sync
    resources.SwapChain().Present(LS_CANVAS_SYNC_INTERVAL);
}

auto report_device_lost(const winrt::hresult_error& exc,
                        const GraphicsResources& resources) -> void {
    // must be called from an exception handler
    Expects(std::current_exception());

    // as we use Win2D ourselves, we have to report device lost as documented here:
    //  https://microsoft.github.io/Win2D/WinUI3/html/HandlingDeviceLost.htm

    if (const auto& swap_chain = resources.SwapChain()) {
        if (const auto device = swap_chain.Device()) {
            if (device.IsDeviceLost(exc.code())) {
                device.RaiseDeviceLost();
            }
        }
    }
}

auto render_and_show_frame(GraphicsResources& resources, const IRenderGuiActions& actions,
                           RenderBufferSink& render_sink) -> void {
    try {
        const auto status = render_sink.draw_buffer(
            [&](const SwapChainParams& params, const frame_t& frame) -> BufferDrawStatus {
                ensure_swap_chain_initialized(resources, params, actions);
                return update_bitmap(resources, params, frame);
            });

        if (status == BufferDrawStatus::DrawingSucceded) {
            render_frame(resources);
            present_frame(resources);
        }
    }

    catch (const winrt::hresult_error& exc) {
        report_device_lost(exc, resources);

        if (DeviceLostException_IsDeviceLost(exc.code())) {
            OutputDebugStringW(L"WARNING: Device lost in render-thread (logicsim).\n");
            resources = GraphicsResources {};
            return;
        }
        if (exc.code() == E_INVALIDARG) {
            OutputDebugStringW(L"WARNING: E_INVALIDARG in render-thread (logicsim).\n");
            resources = GraphicsResources {};
            return;
        }
        throw;
    }
}

auto render_thread_main(std::stop_token token, std::unique_ptr<IRenderGuiActions> actions,
                        RenderBufferSink render_sink) {
    try {
        Expects(actions);
        winrt::init_apartment();

        auto resources = GraphicsResources {};

        try {
            while (!token.stop_requested()) {
                render_and_show_frame(resources, *actions, render_sink);
            }
        } catch (const ShutdownException&) {
            // normal shutdown behavior.
        }

    } catch (const winrt::hresult_error& exc [[maybe_unused]]) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION RENDER-THREAD !!!! {}\n\n");
        std::print("\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! \n\n");
        std::terminate();
    } catch (const std::exception& exc [[maybe_unused]]) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION RENDER-THREAD !!!! {}\n\n");
        std::print("\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! {}\n\n", exc.what());
        std::terminate();
    } catch (...) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION RENDER-THREAD !!!! {}\n\n");
        std::print("\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! \n\n");
        std::terminate();
    }
}

}  // namespace

auto create_render_thread(std::unique_ptr<IRenderGuiActions> actions,
                          RenderBufferSink render_sink) -> std::jthread {
    Expects(actions);

    return std::jthread(render_thread_main, std::move(actions), std::move(render_sink));
}

}  // namespace logicsim
