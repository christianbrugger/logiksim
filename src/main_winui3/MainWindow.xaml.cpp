#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::main_winui3::implementation {

namespace {

/**
 * Change things on the GUI
 *
 * All methods are thread-safe and async.
 */
class RenderGuiActions : public logicsim::IRenderGuiActions {
   public:
    RenderGuiActions(MainWindow& window);

    auto register_swap_chain(winrt::Microsoft::Graphics::Canvas::CanvasSwapChain
                                 swap_chain) const -> void override;

   private:
    weak_ref<MainWindow> window_weak_;
    Microsoft::UI::Dispatching::DispatcherQueue queue_;
};

RenderGuiActions::RenderGuiActions(MainWindow& window)
    : window_weak_ {window.get_weak()}, queue_ {window.DispatcherQueue()} {
    // Make sure this method is called from the UI thread.
    Expects(queue_);
    Expects(queue_.HasThreadAccess());
}

auto RenderGuiActions::register_swap_chain(
    winrt::Microsoft::Graphics::Canvas::CanvasSwapChain swap_chain) const -> void {
    Expects(swap_chain);

    queue_.TryEnqueue([window_weak = window_weak_, swap_chain = std::move(swap_chain)]() {
        if (const auto window = window_weak.get()) {
            window->register_swap_chain(swap_chain);
        }
    });
}

/**
 * Change things on the GUI
 *
 * All methods are thread-safe and async.
 */
class BackendGuiActions : public logicsim::IBackendGuiActions {
   public:
    BackendGuiActions(MainWindow& window);

    auto change_title(hstring title) const -> void override;

   private:
    weak_ref<MainWindow> window_weak_;
    Microsoft::UI::Dispatching::DispatcherQueue queue_;
};

BackendGuiActions::BackendGuiActions(MainWindow& window)
    : window_weak_ {window.get_weak()}, queue_ {window.DispatcherQueue()} {
    // Make sure this method is called from the UI thread.
    Expects(queue_);
    Expects(queue_.HasThreadAccess());
}

auto BackendGuiActions::change_title(hstring title) const -> void {
    queue_.TryEnqueue([window_weak = window_weak_, title_value = hstring {title}]() {
        if (const auto window = window_weak.get()) {
            window->Title(title_value);
        }
    });
}

}  // namespace

auto MainWindow::InitializeComponent() -> void {
    MainWindowT<MainWindow>::InitializeComponent();

    // title
    Title(L"LogikSim");

    // size
    // TODO: handle positon
    // TODO: display scaling
    AppWindow().ResizeClient(
        winrt::Windows::Graphics::SizeInt32 {.Width = 500, .Height = 450});
    // AppWindow().MoveAndResize(winrt::Windows::Graphics::RectInt32 {2400, 300, 500,
    // 450});

    // create threads
    auto buffer_parts = logicsim::create_render_buffer_parts();
    backend_thread_ =
        logicsim::create_backend_thread(std::make_unique<BackendGuiActions>(*this),
                                        backend_tasks_, std::move(buffer_parts.source));
    render_thread_ = logicsim::create_render_thread(
        std::make_unique<RenderGuiActions>(*this), std::move(buffer_parts.sink));
    render_buffer_control_ = std::move(buffer_parts.control);
}

auto MainWindow::CanvasPanel_SizeChanged(IInspectable const& sender [[maybe_unused]],
                                         SizeChangedEventArgs const& args
                                         [[maybe_unused]]) -> void {
    update_render_size();
}

auto MainWindow::CanvasPanel_Loaded(IInspectable const& sender [[maybe_unused]],
                                    RoutedEventArgs const& args
                                    [[maybe_unused]]) -> void {
    update_render_size();

    const auto panel = CanvasPanel();
    Expects(panel);
    const auto xaml_root = panel.XamlRoot();
    Expects(xaml_root);

    //
    // React to DPI changes via the XamlRoot.Changed event.
    //
    // As recommended here:
    // https://github.com/microsoft/WindowsAppSDK/issues/3227#issuecomment-1343065682
    //
    xaml_root.Changed(
        [weak = get_weak()](XamlRoot const&, XamlRootChangedEventArgs const&) {
            if (auto self = weak.get()) {
                self->update_render_size();
            }
        });
}

auto MainWindow::CanvasPanel_PointerMoved(IInspectable const& sender [[maybe_unused]],
                                          Input::PointerRoutedEventArgs const& args)
    -> void {
    const auto p = args.GetCurrentPoint(CanvasPanel()).Position();
    backend_tasks_.push(logicsim::PointDevice {.x = p.X, .y = p.Y});
}

auto MainWindow::register_swap_chain(
    const winrt::Microsoft::Graphics::Canvas::CanvasSwapChain& swap_chain) -> void {
    CanvasPanel().SwapChain(swap_chain);
}

auto MainWindow::update_render_size() -> void {
    const auto panel = CanvasPanel();
    if (!panel) {
        return;
    }
    const auto xaml_root = panel.XamlRoot();
    if (!xaml_root) {
        return;
    }

    const auto size_device = panel.ActualSize();
    const auto dpi_scale = xaml_root.RasterizationScale();

    const auto canvas_params = logicsim::CanvasParams {
        .width_device = size_device.x,
        .height_device = size_device.y,
        .rasterization_scale = dpi_scale,
    };
    const auto params = logicsim::to_swap_chain_params_or_default(canvas_params);

    backend_tasks_.push(params);
}

}  // namespace winrt::main_winui3::implementation
