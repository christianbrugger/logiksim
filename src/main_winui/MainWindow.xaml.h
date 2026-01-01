#pragma once

#include "MainWindow.g.h"
#include "main_winui/src/backend_thread.h"
#include "main_winui/src/ls_key_tracker.h"
#include "main_winui/src/render_buffer.h"
#include "main_winui/src/render_thread.h"

#include <future>
#include <thread>

namespace winrt::main_winui::implementation {

// Window needs to hold references to the IconSources assigned to XamlUICommand otherwise
// they will be deleted.
struct IconSources {
    using IconSource = Microsoft::UI::Xaml::Controls::IconSource;

    IconSource simulation_start_enabled {nullptr};
    IconSource simulation_start_disabled {nullptr};
    IconSource simulation_end_enabled {nullptr};
    IconSource simulation_end_disabled {nullptr};
};

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow(std::optional<std::filesystem::path> path = {});

    auto InitializeComponent() -> void;

    // Window

    auto Window_Closed(Windows::Foundation::IInspectable const& sender,
                       Microsoft::UI::Xaml::WindowEventArgs const& args) -> void;

    // Page

    auto Page_ActualThemeChanged(Microsoft::UI::Xaml::FrameworkElement const& sender,
                                 Windows::Foundation::IInspectable const& args) -> void;

    // Grid

    auto MainGrid_DragOver(IInspectable const&,
                           Microsoft::UI::Xaml::DragEventArgs const& args) -> void;

    auto MainGrid_Drop(IInspectable, Microsoft::UI::Xaml::DragEventArgs args)
        -> Windows::Foundation::IAsyncAction;

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

    // Backend Methods

    auto change_title(const hstring& title) -> void;

    auto register_swap_chain(
        const Microsoft::Graphics::Canvas::CanvasSwapChain& swap_chain) -> void;

    auto config_update(logicsim::exporting::CircuitUIConfig config) -> void;

    using ModalResult = logicsim::exporting::ModalResult;

    auto show_dialog_blocking(logicsim::exporting::ModalRequest request,
                              std::promise<ModalResult> promise)
        -> Windows::Foundation::IAsyncAction;
    auto show_dialog_blocking(logicsim::exporting::SaveCurrentModal request,
                              std::promise<ModalResult> promise)
        -> Windows::Foundation::IAsyncAction;
    auto show_dialog_blocking(logicsim::exporting::OpenFileModal request,
                              std::promise<ModalResult> promise)
        -> Windows::Foundation::IAsyncAction;
    auto show_dialog_blocking(logicsim::exporting::SaveFileModal request,
                              std::promise<ModalResult> promise)
        -> Windows::Foundation::IAsyncAction;
    auto show_dialog_blocking(logicsim::exporting::ErrorMessage message,
                              std::promise<void> promise)
        -> Windows::Foundation::IAsyncAction;

    auto end_modal_state() -> void;

    auto exit_application_no_dialog() -> void;

    // UI Command

    void XamlUICommand_ExecuteRequested(
        Microsoft::UI::Xaml::Input::XamlUICommand const& sender,
        Microsoft::UI::Xaml::Input::ExecuteRequestedEventArgs const&);

    void XamlUICommand_CanExecuteRequest(
        Microsoft::UI::Xaml::Input::XamlUICommand const& sender,
        Microsoft::UI::Xaml::Input::CanExecuteRequestedEventArgs const& args);

   private:
    auto set_modal(bool value) -> void;
    auto update_render_size() -> void;
    auto update_icons_and_button_states() -> void;

   private:
    std::optional<std::filesystem::path> command_line_file_ {};
    bool is_modal_ {};
    bool is_destroyed_ {};

    IconSources icon_sources_ {};
    std::jthread backend_thread_ {};
    std::jthread render_thread_ {};

    // destroy source and control blocks before threads, so shutdown is initiated
    logicsim::RenderBufferControl render_buffer_control_ {};
    logicsim::BackendTaskSource backend_tasks_ {};

    logicsim::KeyTracker key_tracker_ {};
    std::optional<logicsim::exporting::CircuitUIConfig> last_config_ {};
};

}  // namespace winrt::main_winui::implementation

namespace winrt::main_winui::factory_implementation {

struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {
    //
};

}  // namespace winrt::main_winui::factory_implementation
