#pragma once

#include "LayeredFontIcon.g.h"

namespace winrt::main_winui::implementation {

struct LayeredFontIcon : LayeredFontIconT<LayeredFontIcon> {
    using icon_t = winrt::Microsoft::UI::Xaml::Controls::FontIcon;
    using icont_vector_t = Windows::Foundation::Collections::IVector<icon_t>;

    LayeredFontIcon();

    auto OnApplyTemplate() -> void;

    auto FontIcons() -> icont_vector_t;

   private:
    icont_vector_t m_fontIcons {};
};

}  // namespace winrt::main_winui::implementation

namespace winrt::main_winui::factory_implementation {

struct LayeredFontIcon
    : LayeredFontIconT<LayeredFontIcon, implementation::LayeredFontIcon> {
    //
};

}  // namespace winrt::main_winui::factory_implementation