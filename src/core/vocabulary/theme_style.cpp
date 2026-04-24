#include "core/vocabulary/theme_style.h"

namespace logicsim {

template <>
auto format(ThemeStyle style) -> std::string {
    switch (style) {
        using enum ThemeStyle;

        case light:
            return "light";
        case dark:
            return "dark";
    };
    std::terminate();
}

template <>
auto format(ThemeStyleRequest style) -> std::string {
    switch (style) {
        using enum ThemeStyleRequest;

        case system_default:
            return "system_default";
        case light:
            return "light";
        case dark:
            return "dark";
    };
    std::terminate();
}

}  // namespace logicsim
