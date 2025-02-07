#pragma once

#include "MainWindow.g.h"
#include "main_winui/src/backend_thread.h"
#include "main_winui/src/ls_key_tracker.h"
#include "main_winui/src/render_thread.h"

namespace winrt::main_winui::implementation {

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow() {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
    }

    auto InitializeComponent() -> void;

    // CanvasPanel

    auto CanvasPanel_SizeChanged(IInspectable const& sender [[maybe_unused]],
                                 Microsoft::UI::Xaml::SizeChangedEventArgs const& args
                                 [[maybe_unused]]) -> void;
    auto CanvasPanel_Loaded(IInspectable const& sender [[maybe_unused]],
                            Microsoft::UI::Xaml::RoutedEventArgs const& args
                            [[maybe_unused]]) -> void;
    auto CanvasPanel_PointerMoved(
        IInspectable const& sender [[maybe_unused]],
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;
    auto CanvasPanel_PointerPressed(
        IInspectable const& sender [[maybe_unused]],
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;
    auto CanvasPanel_PointerReleased(
        IInspectable const& sender [[maybe_unused]],
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;
    auto CanvasPanel_PointerCanceled(
        IInspectable const& sender [[maybe_unused]],
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;
    auto CanvasPanel_PointerCaptureLost(
        IInspectable const& sender [[maybe_unused]],
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;

    // Public Methods

    auto register_swap_chain(
        const winrt::Microsoft::Graphics::Canvas::CanvasSwapChain& swap_chain) -> void;

   private:
    auto update_render_size() -> void;

   private:
    std::jthread backend_thread_ {};
    std::jthread render_thread_ {};

    // destroy source and control blocks before threads, so shutdown is initiated
    logicsim::BackendTaskSource backend_tasks_ {};
    logicsim::RenderBufferControl render_buffer_control_ {};

    logicsim::KeyTracker key_tracker_ {};
};

}  // namespace winrt::main_winui::implementation

namespace winrt::main_winui::factory_implementation {

struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
    //
};

}  // namespace winrt::main_winui::factory_implementation
