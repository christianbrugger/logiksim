#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "main_winui/src/ls_key_tracker.h"
#include "main_winui/src/ls_overload.h"
#include "main_winui/src/ls_timer.h"
#include "main_winui/src/ls_vocabulary.h"
#include "main_winui/src/ls_xaml_utils.h"

#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <iostream>
#include <print>
#include <string>
#include <variant>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::main_winui::implementation {

namespace {

/**
 * Change things on the GUI
 *
 * All methods are thread-safe and async.
 */
class RenderGuiActions : public logicsim::IRenderGuiActions {
   public:
    RenderGuiActions(MainWindow& window);

    auto register_swap_chain(
        winrt::Microsoft::Graphics::Canvas::CanvasSwapChain swap_chain) const
        -> void override;

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
    auto config_update(logicsim::exporting::CircuitUIConfig config) const
        -> void override;

    [[nodiscard]] auto show_dialog_blocking(logicsim::exporting::ModalRequest request)
        const -> logicsim::exporting::ModalResult override;
    auto show_dialog_blocking(logicsim::exporting::ErrorMessage message) const
        -> void override;
    auto end_modal_state() const -> void override;
    auto exit_application_no_dialog() const -> void override;

   private:
    weak_ref<MainWindow> window_weak_;
    Microsoft::UI::Dispatching::DispatcherQueue queue_;
};

BackendGuiActions::BackendGuiActions(MainWindow& window)
    : window_weak_ {window.get_weak()}, queue_ {window.DispatcherQueue()} {
    // Makes sure this method is called from the UI thread.
    Expects(queue_);
    Expects(queue_.HasThreadAccess());
}

auto BackendGuiActions::change_title(hstring title) const -> void {
    queue_.TryEnqueue([window_weak = window_weak_, title_value = std::move(title)]() {
        if (const auto window = window_weak.get()) {
            window->change_title(title_value);
        }
    });
}

auto BackendGuiActions::config_update(logicsim::exporting::CircuitUIConfig config) const
    -> void {
    queue_.TryEnqueue([window_weak = window_weak_, config_value = std::move(config)]() {
        if (const auto window = window_weak.get()) {
            window->config_update(std::move(config_value));
        }
    });
}

namespace {

template <typename T>
auto get_with_shutdown(std::future<T>& future) {
    Expects(future.valid());
    try {
        // TODO: handle thread shutdown while waiting on future
        return future.get();
    } catch (const std::future_error& exc) {
        if (exc.code() == std::future_errc::broken_promise) {
            throw logicsim::ShutdownException {"Broken promise"};
        }
        throw;
    }
}

}  // namespace

auto BackendGuiActions::show_dialog_blocking(
    logicsim::exporting::ModalRequest request) const -> logicsim::exporting::ModalResult {
    auto promise = std::promise<logicsim::exporting::ModalResult> {};
    auto future = promise.get_future();

    queue_.TryEnqueue([window_weak = window_weak_, request_value = std::move(request),
                       promise_value = std::move(promise)]() mutable {
        if (const auto window = window_weak.get()) {
            window->show_dialog_blocking(std::move(request_value),
                                         std::move(promise_value));
        }
    });

    return (get_with_shutdown(future));
}

auto BackendGuiActions::show_dialog_blocking(
    logicsim::exporting::ErrorMessage message) const -> void {
    auto promise = std::promise<void> {};
    auto future = promise.get_future();

    queue_.TryEnqueue([window_weak = window_weak_, message_value = std::move(message),
                       promise_value = std::move(promise)]() mutable {
        if (const auto window = window_weak.get()) {
            window->show_dialog_blocking(std::move(message_value),
                                         std::move(promise_value));
        }
    });

    return (get_with_shutdown(future));
}

auto BackendGuiActions::end_modal_state() const -> void {
    queue_.TryEnqueue([window_weak = window_weak_]() mutable {
        if (const auto window = window_weak.get()) {
            window->end_modal_state();
        }
    });
}

auto BackendGuiActions::exit_application_no_dialog() const -> void {
    queue_.TryEnqueue([window_weak = window_weak_]() mutable {
        if (const auto window = window_weak.get()) {
            window->exit_application_no_dialog();
        }
    });
}

[[nodiscard]] auto lookup_icons() -> IconSources {
    const auto get_icon = [](std::wstring_view name) {
        return Application::Current()
            .Resources()
            .Lookup(box_value(name))
            .as<Controls::IconSource>();
    };

    return IconSources {
        .simulation_start_enabled = get_icon(L"FontSimulationStartEnabled"),
        .simulation_start_disabled = get_icon(L"FontSimulationStartDisabled"),
        .simulation_end_enabled = get_icon(L"FontSimulationStopEnabled"),
        .simulation_end_disabled = get_icon(L"FontSimulationStopDisabled"),
    };
}

// Notes:
// 1) It is not enough to change the brush or color of the FontIconSource,
//    as other components create icons from the IconSource and the link is
//    lost.
// 2) Strangely the ressource directory does not hold references to the font
// icon
//    sources and are destroyed when assigning a different icon. Thats why we
//    need to store references to the icon sources in the window class.
auto set_simulation_icons(
    MainWindow& w, const IconSources& icons,
    const std::optional<logicsim::exporting::CircuitUIConfig>& config) -> void {
    using namespace logicsim::exporting;

    if (!config.has_value()) {
        w.StartSimulationCommand().IconSource(icons.simulation_start_disabled);
        w.StopSimulationCommand().IconSource(icons.simulation_end_disabled);
    } else if (config.value().state.type == CircuitStateType::Simulation) {
        w.StartSimulationCommand().IconSource(icons.simulation_start_disabled);
        w.StopSimulationCommand().IconSource(icons.simulation_end_enabled);
    } else {
        w.StartSimulationCommand().IconSource(icons.simulation_start_enabled);
        w.StopSimulationCommand().IconSource(icons.simulation_end_disabled);
    }
}

auto clear_simulation_icons(MainWindow& w) -> void {
    set_simulation_icons(w, IconSources {}, std::nullopt);
}

}  // namespace

MainWindow::MainWindow(std::optional<std::filesystem::path> path__)
    : command_line_file_ {std::move(path__)} {
    // Xaml objects should not call InitializeComponent during construction.
    // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
}

auto MainWindow::InitializeComponent() -> void {
    MainWindowT<MainWindow>::InitializeComponent();

    // title bar
    ExtendsContentIntoTitleBar(true);
    SetTitleBar(MainTitleBar());
    change_title(L"");
    AppWindow().TitleBar().PreferredTheme(
        Microsoft::UI::Windowing::TitleBarTheme::UseDefaultAppMode);
    AppWindow().SetIcon(L"resources/icons/derivative/app_icon_256.ico");

    // size
    // TODO: handle positon
    // TODO: display scaling
    AppWindow().ResizeClient(
        winrt::Windows::Graphics::SizeInt32 {.Width = 500, .Height = 450});
    // AppWindow().MoveAndResize(winrt::Windows::Graphics::RectInt32 {2400, 300, 500,
    // 450});

    // test
    auto presenter = Microsoft::UI::Windowing::OverlappedPresenter::Create();
    presenter.PreferredMinimumWidth(400);
    presenter.PreferredMinimumHeight(200);
    AppWindow().SetPresenter(presenter);

    // custom icons
    icon_sources_ = lookup_icons();
    set_simulation_icons(*this, icon_sources_, last_config_);

    // create threads
    auto buffer_parts = logicsim::create_render_buffer_parts();
    auto task_parts = logicsim::create_backend_task_queue_parts();

    backend_thread_ = logicsim::create_backend_thread(
        std::make_unique<BackendGuiActions>(*this), std::move(task_parts.sink),
        std::move(buffer_parts.source));
    render_thread_ = logicsim::create_render_thread(
        std::make_unique<RenderGuiActions>(*this), std::move(buffer_parts.sink));

    render_buffer_control_ = std::move(buffer_parts.control);
    backend_tasks_ = std::move(task_parts.source);

    // open command line file
    if (command_line_file_) {
        set_modal(true);
        backend_tasks_.push(logicsim::OpenFileEvent {
            .filename = std::exchange(command_line_file_, std::nullopt).value(),
        });
    }
}

auto MainWindow::set_modal(bool value) -> void {
    Expects(DispatcherQueue().HasThreadAccess());
    using namespace Microsoft::UI::Windowing;

    if (is_modal_ == value) {
        return;
    }
    is_modal_ = value;

    // gray out icons
    update_icons_and_button_states();
    // disable main window
    ContentControl().IsEnabled(!value);
    // disable resize
    auto presenter = AppWindow().Presenter().as<OverlappedPresenter>();
    if (presenter) {
        presenter.IsMaximizable(!value);
        presenter.IsMinimizable(!value);
        presenter.IsResizable(!value);
    }
}

void MainWindow::Window_Closed(IInspectable const&, WindowEventArgs const& args) {
    // Only if destroyed is set we are destroying the window.
    // This is initiated by the backend.
    if (is_destroyed_) {
        return;
    }
    args.Handled(true);

    if (is_modal_) {
        // ignore any close requests while modal dialog is active.
        return;
    }
    ExitCommand().Execute(nullptr);
}

auto MainWindow::Page_ActualThemeChanged(FrameworkElement const&, IInspectable const&)
    -> void {
    // Icons need to be cleared first as otherwise they are not updated, if the same
    // icon source is set, although now with a different theme color.
    clear_simulation_icons(*this);
    set_simulation_icons(*this, icon_sources_, last_config_);
}

auto MainWindow::MainGrid_DragOver(IInspectable const&, DragEventArgs const& args)
    -> void {
    using namespace winrt::Windows::ApplicationModel::DataTransfer;

    if (!is_modal_ && args.DataView().Contains(StandardDataFormats::StorageItems())) {
        args.AcceptedOperation(DataPackageOperation::Copy);
        args.DragUIOverride().Caption(L"Open File");
    } else {
        args.AcceptedOperation(DataPackageOperation::None);
    }
}

auto MainWindow::MainGrid_Drop(IInspectable, DragEventArgs args)
    -> Windows::Foundation::IAsyncAction {
    using namespace winrt::Windows::ApplicationModel::DataTransfer;

    const auto lifetime [[maybe_unused]] = get_strong();

    if (args.DataView().Contains(StandardDataFormats::StorageItems())) {
        const auto items = co_await args.DataView().GetStorageItemsAsync();

        if (!is_modal_ && items.Size() != 0) {
            set_modal(true);
            backend_tasks_.push(logicsim::OpenFileEvent {
                .filename = std::wstring {items.GetAt(0).Path()}});
        }
    }
}

auto MainWindow::CanvasPanel_SizeChanged(IInspectable const&, SizeChangedEventArgs const&)
    -> void {
    update_render_size();
}

auto MainWindow::CanvasPanel_Loaded(IInspectable const&, RoutedEventArgs const&) -> void {
    update_render_size();

    const auto panel = CanvasPanel();
    Expects(panel);
    const auto xaml_root = panel.XamlRoot();
    Expects(xaml_root);

    // Set intial focus
    panel.Focus(FocusState::Programmatic);

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

auto MainWindow::CanvasPanel_PointerEvent(IInspectable const& sender,
                                          Input::PointerRoutedEventArgs const& args)
    -> void {
    using namespace logicsim;
    using namespace winrt::Microsoft::UI::Input;

    const auto point = args.GetCurrentPoint(CanvasPanel());

    if (point.PointerDeviceType() != PointerDeviceType::Mouse) {
        return;
    }

    // generate events for the backend
    const auto data = PointerEventData {point, args.KeyModifiers()};
    key_tracker_.submit_event(data, backend_tasks_);

    // steal focus
    if (is_pressed_kind(point.Properties().PointerUpdateKind())) {
        CanvasPanel().Focus(FocusState::Pointer);
    }
    sender.as<UIElement>().CapturePointer(args.Pointer());
    args.Handled(true);
}

auto MainWindow::CanvasPanel_PointerWheelChanged(
    IInspectable const&, Input::PointerRoutedEventArgs const& args) -> void {
    using namespace logicsim;
    using namespace winrt::Microsoft::UI::Input;

    const auto point = args.GetCurrentPoint(CanvasPanel());

    if (point.PointerDeviceType() != PointerDeviceType::Mouse) {
        return;
    }

    backend_tasks_.push(exporting::MouseWheelEvent {
        .position = to_device_position(point),
        .angle_delta = to_angle_delta(point),
        .modifiers = to_keyboard_modifiers(args.KeyModifiers()),
    });

    args.Handled(true);
}

void MainWindow::CanvasPanel_KeyDown(IInspectable const&,
                                     Input::KeyRoutedEventArgs const& args) {
    using namespace winrt::Windows;
    using namespace logicsim;

    const auto key = args.Key();

    // ignore repeat keys
    if (args.KeyStatus().WasKeyDown) {
        return;
    }

    if (key == System::VirtualKey::Enter) {
        backend_tasks_.push(exporting::VirtualKey::Enter);
        args.Handled(true);
    }
    if (key == System::VirtualKey::Escape) {
        backend_tasks_.push(exporting::VirtualKey::Escape);
        args.Handled(true);
    }
}

auto MainWindow::change_title(const hstring& value) -> void {
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        return;
    }

    const auto app_title = L"LogikSim";
    const auto title = value.empty() ? app_title : value + L" - " + app_title;

    MainTitleBar().Title(title);
    Title(title);
}

auto MainWindow::register_swap_chain(
    const winrt::Microsoft::Graphics::Canvas::CanvasSwapChain& swap_chain) -> void {
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        return;
    }

    CanvasPanel().SwapChain(swap_chain);
}

auto MainWindow::config_update(logicsim::exporting::CircuitUIConfig config__) -> void {
    using namespace logicsim::exporting;
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        return;
    }

    // last_config_ needs to be set first, as notify handlers fire immediately.
    const auto last_config = std::exchange(last_config_, std::move(config__));
    const auto& new_config = last_config_.value();

    // simulation state
    if (!last_config.has_value() ||
        ((last_config->state.type == CircuitStateType::Simulation) !=
         (new_config.state.type == CircuitStateType::Simulation))) {
        update_icons_and_button_states();
    }

    // Toggles

    WireDelayFlyout().IsChecked(new_config.simulation.use_wire_delay);
    BenchmarkFlyout().IsChecked(new_config.render.do_benchmark);
    DebugInfoDialogFlyout().IsChecked(false);
    ShowCircuitFlyout().IsChecked(new_config.render.show_circuit);
    ShowCollisionIndexFlyout().IsChecked(new_config.render.show_collision_index);
    ShowConnectionIndexFlyout().IsChecked(new_config.render.show_connection_index);
    ShowSelectionIndexFlyout().IsChecked(new_config.render.show_selection_index);
    ShowRenderBordersFlyout().IsChecked(new_config.render.show_render_borders);
    ShowMousePositionFlyout().IsChecked(new_config.render.show_mouse_position);
    EnterNonInteractiveModeFlyout().IsChecked(new_config.state.type ==
                                              CircuitStateType::NonInteractive);
    JitRenderingFlyout().IsChecked(new_config.render.jit_rendering);

    // Radios
    [&] {
        switch (new_config.render.wire_render_style) {
            case WireRenderStyle::red:
                WireStyleRedFlyout().IsChecked(true);
                return;
            case WireRenderStyle::bold:
                WireStyleBoldFlyout().IsChecked(true);
                return;
            case WireRenderStyle::bold_red:
                WireStyleBoldRedFlyout().IsChecked(true);
                return;
        };
        std::terminate();
    }();
    [&] {
        switch (new_config.render.thread_count) {
            case ThreadCount::synchronous:
                ThreadSyncFlyout().IsChecked(true);
                return;
            case ThreadCount::two:
                Thread2Flyout().IsChecked(true);
                return;
            case ThreadCount::four:
                Thread4Flyout().IsChecked(true);
                return;
            case ThreadCount::eight:
                Thread8Flyout().IsChecked(true);
                return;
        };
        std::terminate();
    }();
}

auto MainWindow::show_dialog_blocking(logicsim::exporting::ModalRequest request,
                                      std::promise<ModalResult> promise)
    -> Windows::Foundation::IAsyncAction {
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        co_return;
    }

    co_await std::visit(
        [&](auto&& value) -> Windows::Foundation::IAsyncAction {
            co_await this->show_dialog_blocking(std::move(value), std::move(promise));
        },
        std::move(request));
}

namespace {

[[nodiscard]] auto to_modal_result(Controls::ContentDialogResult result)
    -> logicsim::exporting::ModalResult {
    using namespace winrt::Microsoft::UI::Xaml::Controls;

    switch (result) {
        case ContentDialogResult::None:
            return logicsim::exporting::SaveCurrentCancel {};
        case ContentDialogResult::Primary:
            return logicsim::exporting::SaveCurrentYes {};
        case ContentDialogResult::Secondary:
            return logicsim::exporting::SaveCurrentNo {};
    };
    std::terminate();
};

}  // namespace

auto MainWindow::show_dialog_blocking(logicsim::exporting::SaveCurrentModal request,
                                      std::promise<ModalResult> promise)
    -> Windows::Foundation::IAsyncAction {
    using namespace winrt::Microsoft::UI::Xaml::Controls;
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        co_return;
    }

    const auto lifetime [[maybe_unused]] = get_strong();

    try {
        auto dialog = ContentDialog {};
        dialog.XamlRoot(Content().XamlRoot());

        dialog.Style(Application::Current()
                         .Resources()
                         .Lookup(box_value(L"DefaultContentDialogStyle"))
                         .as<Style>());

        dialog.Title(box_value(L"LogikSim"));
        dialog.PrimaryButtonText(L"Save");
        dialog.SecondaryButtonText(L"Don't save");
        dialog.CloseButtonText(L"Cancel");
        dialog.Content(
            box_value(L"Do you want to save changes to " + request.filename.native()));
        dialog.DefaultButton(ContentDialogButton::Primary);

        set_modal(true);
        const auto result = co_await dialog.ShowAsync();

        promise.set_value(to_modal_result(result));
    } catch (...) {
        promise.set_exception(std::current_exception());
    }
    co_return;
}

auto MainWindow::show_dialog_blocking(logicsim::exporting::OpenFileModal request,
                                      std::promise<ModalResult> promise)
    -> Windows::Foundation::IAsyncAction {
    using namespace winrt::Microsoft::Windows::Storage::Pickers;
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        co_return;
    }

    const auto lifetime [[maybe_unused]] = get_strong();

    try {
        const auto window_id = AppWindow().Id();
        auto picker = FileOpenPicker {window_id};

        picker.SuggestedStartLocation(PickerLocationId::DocumentsLibrary);
        picker.ViewMode(PickerViewMode::List);
        picker.FileTypeFilter().Append(L".ls2");

        const auto result = co_await picker.PickSingleFileAsync();

        if (result) {
            promise.set_value(logicsim::exporting::OpenFileOpen {
                .filename = std::filesystem::path {std::wstring {result.Path()}},
            });
        } else {
            promise.set_value(logicsim::exporting::OpenFileCancel {});
        }
    } catch (...) {
        promise.set_exception(std::current_exception());
    }
    co_return;
}

auto MainWindow::show_dialog_blocking(logicsim::exporting::SaveFileModal request,
                                      std::promise<ModalResult> promise)
    -> Windows::Foundation::IAsyncAction {
    using namespace Microsoft::Windows::Storage::Pickers;
    using namespace Windows::Foundation::Collections;
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        co_return;
    }

    const auto lifetime [[maybe_unused]] = get_strong();

    try {
        const auto window_id = AppWindow().Id();
        auto picker = FileSavePicker {window_id};

        picker.SuggestedStartLocation(PickerLocationId::DocumentsLibrary);
        picker.FileTypeChoices().Insert(L"Circuit Files",
                                        single_threaded_vector<hstring>({L".ls2"}));
        picker.DefaultFileExtension(L".ls2");

        picker.SuggestedFileName(request.filename.has_filename()
                                     ? request.filename.filename().native()
                                     : L"Circuit");
        auto folder_name = [&] {
            auto v = std::filesystem::path {request.filename};
            v.remove_filename();
            return v;
        }();
        picker.SuggestedFolder(folder_name.native());

        const auto result = co_await picker.PickSaveFileAsync();

        if (result) {
            promise.set_value(logicsim::exporting::SaveFileSave {
                .filename = std::filesystem::path {std::wstring {result.Path()}},
            });
        } else {
            promise.set_value(logicsim::exporting::SaveFileCancel {});
        }
    }

    catch (...) {
        promise.set_exception(std::current_exception());
    }

    co_return;
}

auto MainWindow::show_dialog_blocking(logicsim::exporting::ErrorMessage message,
                                      std::promise<void> promise)
    -> Windows::Foundation::IAsyncAction {
    using namespace winrt::Microsoft::UI::Xaml::Controls;
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        co_return;
    }

    const auto lifetime [[maybe_unused]] = get_strong();

    try {
        auto dialog = ContentDialog {};
        dialog.XamlRoot(Content().XamlRoot());

        dialog.Style(Application::Current()
                         .Resources()
                         .Lookup(box_value(L"DefaultContentDialogStyle"))
                         .as<Style>());

        const auto title =
            std::visit(logicsim::overload(
                           [](const logicsim::exporting::SaveFileError&) -> hstring {
                               return L"LogikSim cannot save this file.";
                           },
                           [](const logicsim::exporting::OpenFileError&) -> hstring {
                               return L"LogikSim cannot open this file.";
                           }),
                       message);

        const auto content = std::visit(
            logicsim::overload(
                [](const logicsim::exporting::SaveFileError& error) -> hstring {
                    return hstring {error.filename.native()};
                },
                [](const logicsim::exporting::OpenFileError& error) -> hstring {
                    if (error.message.empty()) {
                        return hstring {error.filename.native()};
                    }
                    return error.filename.native() + L"\n\n" + to_hstring(error.message);
                }),
            message);

        dialog.Title(box_value(title));
        dialog.PrimaryButtonText(L"OK");
        dialog.IsSecondaryButtonEnabled(false);
        dialog.CloseButtonText(L"Cancel");
        dialog.Content(box_value(content));
        dialog.DefaultButton(ContentDialogButton::Primary);

        set_modal(true);
        co_await dialog.ShowAsync();

        promise.set_value();
    } catch (...) {
        promise.set_exception(std::current_exception());
    }
    co_return;
}

auto MainWindow::end_modal_state() -> void {
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        return;
    }

    set_modal(false);
}

auto MainWindow::exit_application_no_dialog() -> void {
    Expects(DispatcherQueue().HasThreadAccess());
    if (is_destroyed_) {
        return;
    }

    is_destroyed_ = true;
    this->Close();
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

void MainWindow::XamlUICommand_ExecuteRequested(Input::XamlUICommand const& sender,
                                                Input::ExecuteRequestedEventArgs const&) {
    using namespace logicsim;
    using namespace logicsim::exporting;

    const auto get_position = [&] -> std::optional<ls_point_device_fine_t> {
        return logicsim::get_cursor_position(CanvasPanel()).transform([](auto val) {
            return logicsim::to_device_position(val);
        });
    };

    //
    // File
    //

    if (sender == NewCommand()) {
        if (is_modal_) {
            return;
        }
        set_modal(true);
        backend_tasks_.push(FileAction::new_file);
        return;
    }
    if (sender == OpenCommand()) {
        if (is_modal_) {
            return;
        }
        set_modal(true);
        backend_tasks_.push(FileAction::open_file);
        return;
    }
    if (sender == SaveCommand()) {
        if (is_modal_) {
            return;
        }
        set_modal(true);
        backend_tasks_.push(FileAction::save_file);
        return;
    }
    if (sender == SaveAsCommand()) {
        if (is_modal_) {
            return;
        }
        set_modal(true);
        backend_tasks_.push(FileAction::save_as_file);
        return;
    }
    if (sender == ExitCommand()) {
        if (is_modal_) {
            return;
        }
        set_modal(true);
        backend_tasks_.push(FileAction::exit_application);
        return;
    }

    //
    // Edit
    //

    if (sender == UndoCommand()) {
        backend_tasks_.push(UserActionEvent {
            .action = UserAction::undo,
        });
        return;
    }
    if (sender == RedoCommand()) {
        backend_tasks_.push(UserActionEvent {
            .action = UserAction::redo,
        });
        return;
    }
    if (sender == CutCommand()) {
        // backend_tasks_.push(UserActionEvent {
        //     .action = UserAction::cut_selected,
        //     .position = get_position(),
        // });
        return;
    }
    if (sender == CopyCommand()) {
        // backend_tasks_.push(UserActionEvent {
        //     .action = UserAction::copy_selected,
        //     .position = get_position(),
        // });
        return;
    }
    if (sender == PasteCommand()) {
        // backend_tasks_.push(UserActionEvent {
        //     .action = UserAction::paste_from_clipboard,
        //     .position = get_position(),
        // });
        return;
    }
    if (sender == DeleteCommand()) {
        backend_tasks_.push(UserActionEvent {
            .action = UserAction::delete_selected,
        });
        return;
    }
    if (sender == SelectAllCommand()) {
        backend_tasks_.push(UserActionEvent {
            .action = UserAction::select_all,
        });
        return;
    }

    //
    // View
    //

    if (sender == ZoomInCommand()) {
        backend_tasks_.push(UserActionEvent {
            .action = UserAction::zoom_in,
            .position = get_position(),
        });
        return;
    }
    if (sender == ZoomOutCommand()) {
        backend_tasks_.push(UserActionEvent {
            .action = UserAction::zoom_out,
            .position = get_position(),
        });
        return;
    }
    if (sender == ResetZoomCommand()) {
        backend_tasks_.push(UserActionEvent {.action = UserAction::reset_view});
        return;
    }
    {
        if (sender == WireStyleRedCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .render =
                    {
                        .wire_render_style = WireRenderStyle::red,
                    },
            });
            return;
        }
        if (sender == WireStyleBoldCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .render =
                    {
                        .wire_render_style = WireRenderStyle::bold,
                    },
            });
            return;
        }
        if (sender == WireStyleBoldRedCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .render =
                    {
                        .wire_render_style = WireRenderStyle::bold_red,
                    },
            });
            return;
        }
    }

    //
    // Simulation
    //

    if (sender == StartSimulationCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .state =
                {
                    .type = CircuitStateType::Simulation,
                },
        });
        return;
    }
    if (sender == StopSimulationCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .state =
                {
                    .type = CircuitStateType::Editing,
                    .editing_default_mouse_action = DefaultMouseAction::selection,
                },
        });
        return;
    }
    if (sender == WireDelayCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .simulation =
                {
                    .use_wire_delay =
                        !last_config_.value_or({}).simulation.use_wire_delay,
                },
        });
        return;
    }
    {
        using namespace std::chrono_literals;

        if (sender == SimulationSpeed0nsCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {0us},
                    },
            });
            return;
        }
        if (sender == SimulationSpeed1usCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {1us},
                    },
            });
            return;
        }
        if (sender == SimulationSpeed10usCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {10us},
                    },
            });
            return;
        }
        if (sender == SimulationSpeed100usCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {100us},
                    },
            });
            return;
        }
        if (sender == SimulationSpeed1msCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {1ms},
                    },
            });
            return;
        }
        if (sender == SimulationSpeed10msCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {10ms},
                    },
            });
            return;
        }
        if (sender == SimulationSpeed100msCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {100ms},
                    },
            });
            return;
        }
        if (sender == SimulationSpeed1sCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {1s},
                    },
            });
            return;
        }
        if (sender == SimulationSpeed10sCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .simulation =
                    {
                        .simulation_time_rate = time_rate_t {10s},
                    },
            });
            return;
        }
    }

    //
    // Debug
    //

    if (sender == BenchmarkCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .render =
                {
                    .do_benchmark = !last_config_.value_or({}).render.do_benchmark,
                },
        });
        return;
    }
    if (sender == DebugInfoDialogCommand()) {
        std::cout << "TODO: debug info dialog" << '\n';
        return;
    }

    if (sender == ShowCircuitCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .render =
                {
                    .show_circuit = !last_config_.value_or({}).render.show_circuit,
                },
        });
        return;
    }
    if (sender == ShowCollisionIndexCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .render =
                {
                    .show_collision_index =
                        !last_config_.value_or({}).render.show_collision_index,
                },
        });
        return;
    }
    if (sender == ShowConnectionIndexCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .render =
                {
                    .show_connection_index =
                        !last_config_.value_or({}).render.show_connection_index,
                },
        });
        return;
    }
    if (sender == ShowSelectionIndexCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .render =
                {
                    .show_selection_index =
                        !last_config_.value_or({}).render.show_selection_index,
                },
        });
        return;
    }

    if (sender == ReloadCommand()) {
        backend_tasks_.push(UserActionEvent {
            .action = UserAction::reload_circuit,
        });
        return;
    }
    if (sender == ExampleSimpleCommand()) {
        backend_tasks_.push(FileAction::load_example_simple);
        return;
    }
    if (sender == ExampleWiresCommand()) {
        backend_tasks_.push(FileAction::load_example_wires);
        return;
    }
    if (sender == ExampleElementsCommand()) {
        backend_tasks_.push(FileAction::load_example_elements);
        return;
    }
    if (sender == ExampleElementsWiresCommand()) {
        backend_tasks_.push(FileAction::load_example_elements_wires);
        return;
    }

    if (sender == ShowRenderBordersCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .render =
                {
                    .show_render_borders =
                        !last_config_.value_or({}).render.show_render_borders,
                },
        });
        return;
    }
    if (sender == ShowMousePositionCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .render =
                {
                    .show_mouse_position =
                        !last_config_.value_or({}).render.show_mouse_position,
                },
        });
        return;
    }
    if (sender == EnterNonInteractiveModeCommand()) {
        if (last_config_.value_or({}).state.type != CircuitStateType::NonInteractive) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .state =
                    {
                        .type = CircuitStateType::NonInteractive,
                    },
            });
        } else {
            backend_tasks_.push(CircuitUIConfigEvent {
                .state =
                    {
                        .type = CircuitStateType::Editing,
                        .editing_default_mouse_action = DefaultMouseAction::selection,
                    },
            });
        }
        return;
    }

    if (sender == JitRenderingCommand()) {
        backend_tasks_.push(CircuitUIConfigEvent {
            .render =
                {
                    .jit_rendering = !last_config_.value_or({}).render.jit_rendering,
                },
        });
        return;
    }

    {
        if (sender == RenderSynchronousCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .render =
                    {
                        .thread_count = ThreadCount::synchronous,
                    },
            });
            return;
        }
        if (sender == Render2ThreadsCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .render =
                    {
                        .thread_count = ThreadCount::two,
                    },
            });
            return;
        }
        if (sender == Render4ThreadsCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .render =
                    {
                        .thread_count = ThreadCount::four,
                    },
            });
            return;
        }
        if (sender == Render8ThreadsCommand()) {
            backend_tasks_.push(CircuitUIConfigEvent {
                .render =
                    {
                        .thread_count = ThreadCount::eight,
                    },
            });
            return;
        }
    }
}

void MainWindow::XamlUICommand_CanExecuteRequest(
    Input::XamlUICommand const& sender, Input::CanExecuteRequestedEventArgs const& args) {
    if (sender == StartSimulationCommand()) {
        args.CanExecute(!is_modal_ && last_config_.has_value() &&
                        last_config_->state.type !=
                            logicsim::exporting::CircuitStateType::Simulation);
        return;
    }
    if (sender == StopSimulationCommand()) {
        args.CanExecute(!is_modal_ && last_config_.has_value() &&
                        last_config_->state.type ==
                            logicsim::exporting::CircuitStateType::Simulation);
        return;
    }
}

auto MainWindow::update_icons_and_button_states() -> void {
    if (is_modal_) {
        set_simulation_icons(*this, icon_sources_, std::nullopt);
    } else {
        set_simulation_icons(*this, icon_sources_, last_config_);
    }
    StartSimulationCommand().NotifyCanExecuteChanged();
    StopSimulationCommand().NotifyCanExecuteChanged();
}

}  // namespace winrt::main_winui::implementation
