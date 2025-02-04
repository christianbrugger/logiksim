#pragma once

#include "App.xaml.g.h"

namespace winrt::main_winui::implementation {

struct App : AppT<App> {
    App();

    void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

   private:
    winrt::Microsoft::UI::Xaml::Window window {nullptr};
};

}  // namespace winrt::main_winui::implementation
