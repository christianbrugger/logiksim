#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "core_export/logicsim_core_export.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::main_winui3::implementation {
int32_t MainWindow::MyProperty() {
    throw hresult_not_implemented();
}

void MainWindow::MyProperty(int32_t /* value */) {
    throw hresult_not_implemented();  // 14
}

void MainWindow::myButton_Click(IInspectable const&, RoutedEventArgs const&) {
    const auto _ = logicsim::core::CircuitInterface {};

    myButton().Content(box_value(L"Clicked"));
}

}  // namespace winrt::main_winui3::implementation
