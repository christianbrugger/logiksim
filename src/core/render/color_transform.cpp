#include "core/render/color_transform.h"

#include <fmt/format.h>

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

auto Oklch::format() const -> std::string {
    return fmt::format("Oklch({}, {}, {})", l, c, h);
}

auto Oklh::format() const -> std::string {
    return fmt::format("Oklh({}, {})", l, h);
}

namespace details::ct {

auto ab_norm_t::format() const -> std::string {
    return fmt::format("ab_norm_t({}, {}, {})", c_, a_norm_, b_norm_);
};

auto AngleColor::format() const -> std::string {
    return fmt::format("({}, {})", beta, color);
};

}  // namespace details::ct

}  // namespace logicsim
