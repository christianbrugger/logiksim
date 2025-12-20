#pragma once

#include "MainWindow.g.h"
#include "main_winui/LayeredFontIcon.h"
#include "main_winui/src/backend_thread.h"
#include "main_winui/src/ls_key_tracker.h"
#include "main_winui/src/render_buffer.h"
#include "main_winui/src/render_thread.h"

#include <thread>

namespace winrt::main_winui::implementation {

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow() {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
    }

    auto InitializeComponent() -> void;

    // myButton

    auto myButton_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&)
        -> void;

    // CanvasPanel

    auto CanvasPanel_SizeChanged(IInspectable const&,
                                 Microsoft::UI::Xaml::SizeChangedEventArgs const&)
        -> void;
    auto CanvasPanel_Loaded(IInspectable const&,
                            Microsoft::UI::Xaml::RoutedEventArgs const&) -> void;

    auto CanvasPanel_PointerEvent(
        IInspectable const& sender,
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;

    auto CanvasPanel_PointerWheelChanged(
        IInspectable const&,
        Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) -> void;

    void CanvasPanel_KeyDown(IInspectable const&,
                             Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args);

    // Public Methods

    auto register_swap_chain(
        const Microsoft::Graphics::Canvas::CanvasSwapChain& swap_chain) -> void;

    // UI Command

    void XamlUICommand_ExecuteRequested(
        Microsoft::UI::Xaml::Input::XamlUICommand const& sender,
        Microsoft::UI::Xaml::Input::ExecuteRequestedEventArgs const&);

   private:
    auto update_render_size() -> void;

   private:
    std::jthread backend_thread_ {};
    std::jthread render_thread_ {};

    // destroy source and control blocks before threads, so shutdown is initiated
    logicsim::RenderBufferControl render_buffer_control_ {};
    logicsim::BackendTaskSource backend_tasks_ {};

    logicsim::KeyTracker key_tracker_ {};
};

}  // namespace winrt::main_winui::implementation

namespace winrt::main_winui::factory_implementation {

struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
    //
};

}  // namespace winrt::main_winui::factory_implementation
