#include "pch.h"

#include "App.xaml.h"

#include "MainWindow.xaml.h"

#include <gsl/gsl>

#include <processenv.h>

#include <filesystem>
#include <optional>
#include <string>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::main_winui::implementation {

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App() {
    // Xaml objects should not call InitializeComponent during construction.
    // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e) {
        if (IsDebuggerPresent()) {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

namespace {

auto attach_console() -> void {
    Expects(AllocConsole());
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
}

[[nodiscard]] auto get_command_line_argument(int index) -> std::optional<std::wstring> {
    Expects(index >= 0);

    int argc = 0;
    PWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    auto _ = gsl::final_action([&] { LocalFree(argv); });

    if (argv && argc > index) {
        return std::wstring {argv[index]};
    }
    return std::nullopt;
}

[[nodiscard]] auto get_command_line_path() -> std::optional<std::filesystem::path> {
    const auto index = 1;

    if (const auto arg = get_command_line_argument(index)) {
        const auto path = std::filesystem::path {arg.value()};
        if (std::filesystem::is_regular_file(path)) {
            return path;
        }
    }

    return std::nullopt;
}

}  // namespace

/// <summary>
/// Invoked when the application is launched.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e) {
    attach_console();

    window = make<MainWindow>(get_command_line_path());
    window.Activate();
}

}  // namespace winrt::main_winui::implementation
