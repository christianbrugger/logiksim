#include "pch.h"

#include "LayeredFontIcon.h"
#if __has_include("LayeredFontIcon.g.cpp")
#include "LayeredFontIcon.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt::main_winui::implementation {

LayeredFontIcon::LayeredFontIcon() : m_fontIcons {single_threaded_vector<icon_t>()} {
    //
}

auto LayeredFontIcon::OnApplyTemplate() -> void {
    LayeredFontIconT<LayeredFontIcon>::OnApplyTemplate();

    if (VisualTreeHelper::GetChildrenCount(*this) > 0) {
        auto root = VisualTreeHelper::GetChild(*this, 0);

        if (auto grid = root.try_as<Grid>()) {
            grid.Children().Clear();
            for (auto const& icon : m_fontIcons) {
                grid.Children().Append(icon);
            }
        }
    }
}

auto LayeredFontIcon::FontIcons() -> icont_vector_t {
    return m_fontIcons;
}

}  // namespace winrt::main_winui::implementation