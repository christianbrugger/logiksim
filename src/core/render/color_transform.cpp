#include "core/render/color_transform.h"

namespace logicsim {

auto Rgb::format() const -> std::string {
    return fmt::format("Rgb({}, {}, {})", r, g, b);
}

auto Lrgb::format() const -> std::string {
    return fmt::format("Lrgb({}, {}, {})", r, g, b);
}

auto Oklab::format() const -> std::string {
    return fmt::format("Oklab({}, {}, {})", l, a, b);
}

}  // namespace logicsim
