#ifndef LOGICSIM_CORE_RENDER_COLOR_TRANSFORM_H
#define LOGICSIM_CORE_RENDER_COLOR_TRANSFORM_H

#include "core/algorithm/constexpr_math.h"
#include "core/container/static_vector.h"
#include "core/format/struct.h"

#include <limits>
#include <numbers>

// TODO: test to_light of RGB(10, 10, 10)

/*
 * @brief: Define oklab color transformations.
 *
 * Source: https://bottosson.github.io/posts/gamutclipping/
 * Source: https://en.wikipedia.org/wiki/SRGB
 */

namespace logicsim {

namespace defaults {

constexpr static auto dark_mode_gray = std::uint8_t {0x12};

}

/*
 * @brief: rgb color space.
 *
 * Values are within [0, 255]
 */
struct RgbI {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    [[nodiscard]] constexpr auto operator==(const RgbI&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

/*
 * @brief: rgb color space.
 *
 * sRGB values are within [0, 255], but outside are allowed as well.
 */
struct Rgb {
    double r;
    double g;
    double b;

    [[nodiscard]] constexpr auto operator==(const Rgb&) const -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const Rgb&) const = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] constexpr auto is_close(const Rgb& x, const Rgb& y) -> bool;

/*
 * @brief: Linear rgb color space.
 *
 * This means values of sRGB are within [0, 1], but values outside are allowed as well.
 */
struct Lrgb {
    double r;
    double g;
    double b;

    [[nodiscard]] constexpr auto operator==(const Lrgb&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] constexpr auto is_close(const Lrgb& x, const Lrgb& y) -> bool;

/*
 * @brief: Oklab color space
 */
struct Oklab {
    double l;  // lightness
    double a;
    double b;

    [[nodiscard]] constexpr auto operator==(const Oklab&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] constexpr auto is_close(const Oklab& x, const Oklab& y) -> bool;
[[nodiscard]] constexpr auto distance(const Oklab& x, const Oklab& y) -> double;

/*
 * @brief: Oklch color space
 */
struct Oklch {
    double l;  // lightness
    double c;  // chroma
    double h;  // hue in degree

    [[nodiscard]] constexpr auto operator==(const Oklch&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

/*
 * @brief: Oklh
 */
struct Oklh {
    double l;  // lightness
    double h;  // hue in degree

    [[nodiscard]] constexpr auto operator==(const Oklh&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] constexpr auto is_close(const Oklch& x, const Oklch& y) -> bool;

[[nodiscard]] constexpr auto to_rgbi(Rgb c) -> RgbI;
[[nodiscard]] constexpr auto to_rgb(RgbI c) -> Rgb;

[[nodiscard]] constexpr auto to_lrgb(Rgb c) -> Lrgb;
[[nodiscard]] constexpr auto to_rgb(Lrgb c) -> Rgb;

[[nodiscard]] constexpr auto to_oklab(Lrgb c) -> Oklab;
[[nodiscard]] constexpr auto to_lrgb(Oklab c) -> Lrgb;

[[nodiscard]] constexpr auto to_oklch(Oklab c) -> Oklch;
[[nodiscard]] constexpr auto to_oklab(Oklch lch) -> Oklab;

[[nodiscard]] constexpr auto to_dark_mode_raw(Rgb rgb) -> Rgb;
[[nodiscard]] constexpr auto to_light_mode_raw(Rgb rgb) -> Rgb;

//
// Implementation
//

namespace defaults::ct {
// used for comparing two color values
constexpr inline static auto is_close_epsilon = 1e-14;

// precesion at which lrgb is still valid outside [0,1]
constexpr inline static auto lrgb_is_valid_srgb_epsilon = 1e-14;

// band around [0/l_dark, 1] at which chroma is considered 0
constexpr inline static auto luminance_zone_with_zero_chroma = 1e-5;

// assume chroma to be zero below this value in calculations
constexpr inline static auto min_valid_chroma = 1e-6;

// minimum max circle up/down angle for calulations
constexpr inline static auto min_max_chroma_angle_rad = 1e-6 * (std::numbers::pi / 2.);

//
// Affects colors
//

constexpr inline static auto beta_down_max_deg = 90;  // degree
constexpr inline static auto beta_up_max_deg = 80;    // degree

//
// Affects computation times & accuary
//

// number of angles to test to search for sign changes
constexpr inline static auto beta_down_count = 90;
constexpr inline static auto beta_up_count = 80;
// numer of steps to devide angle interval
constexpr inline static auto beta_refinement_steps = std::numeric_limits<double>::digits;

static_assert(std::is_same_v<decltype(beta_refinement_steps), const int>);
static_assert(beta_refinement_steps >= 0);
static_assert(beta_refinement_steps <= std::numeric_limits<double>::digits);

}  // namespace defaults::ct

constexpr auto is_close(const Rgb& x, const Rgb& y) -> bool {
    constexpr auto tol = 255.0 * defaults::ct::is_close_epsilon;
    return cmath::abs(x.r - y.r) < tol &&  //
           cmath::abs(x.g - y.g) < tol &&  //
           cmath::abs(x.b - y.b) < tol;
}

constexpr auto is_close(const Lrgb& x, const Lrgb& y) -> bool {
    constexpr auto tol = defaults::ct::is_close_epsilon;
    return cmath::abs(x.r - y.r) < tol &&  //
           cmath::abs(x.g - y.g) < tol &&  //
           cmath::abs(x.b - y.b) < tol;
}

constexpr auto is_close(const Oklab& x, const Oklab& y) -> bool {
    constexpr auto tol = defaults::ct::is_close_epsilon;
    return cmath::abs(x.l - y.l) < tol &&  //
           cmath::abs(x.a - y.a) < tol &&  //
           cmath::abs(x.b - y.b) < tol;
}

constexpr auto distance(const Oklab& x, const Oklab& y) -> double {
    return cmath::hypot(  //
        x.l - y.l,        //
        x.a - y.a,        //
        x.b - y.b         //
    );
}

constexpr auto is_close(const Oklch& x, const Oklch& y) -> bool {
    constexpr auto tol_lc = defaults::ct::is_close_epsilon;
    constexpr auto tol_h = 360. * defaults::ct::is_close_epsilon;

    if (cmath::abs(x.l - y.l) >= tol_lc) {
        return false;
    }
    if (cmath::abs(x.c - y.c) >= tol_lc) {
        return false;
    }
    if (cmath::abs(x.c) < tol_lc &&  //
        cmath::abs(y.c) < tol_lc) {
        return true;
    }

    const auto diff = cmath::abs(cmath::fmod(x.h - y.h, 360.));
    return (diff < tol_h) || (360. - diff < tol_h);
}

constexpr auto to_rgbi(Rgb c) -> RgbI {
    return RgbI {
        .r = static_cast<std::uint8_t>(cmath::clamp(cmath::round(c.r), 0., 255.)),
        .g = static_cast<std::uint8_t>(cmath::clamp(cmath::round(c.g), 0., 255.)),
        .b = static_cast<std::uint8_t>(cmath::clamp(cmath::round(c.b), 0., 255.)),
    };
}

constexpr auto to_rgb(RgbI c) -> Rgb {
    return Rgb {
        .r = static_cast<double>(c.r),
        .g = static_cast<double>(c.g),
        .b = static_cast<double>(c.b),
    };
}

namespace details::ct {

/*
 * Standard sRGB formulas introduce discontinuity & inversion errors.
 *
 * This creates problems if we use any solvers using these conversions.
 *
 * More precise cutoff points are calculated as following:
 *
 *    from mpmath import mp, mpf
 *    mp.dps = 50
 *    f = lambda x: x / mpf("12.92") - ((x + mpf("0.055")) / mpf("1.055")) ** mpf("2.4")
 *    root = mp.findroot(f, mpf("0.04045"))
 *    print(root)
 *    print(root / mpf("12.92"))
 *
 * Source: https://entropymine.com/imageworsener/srgbformula/
 */

// usually just: 0.04045
constexpr auto LRGB_CUTOFF = 0.040448236277108191704308800334258853909149966736524;
// usually just: 0.0031308
constexpr auto RGB_CUTOFF = 0.0031306684425006340328412384159643075781075825647465;

constexpr auto to_lrgb(double value) -> double {
    const auto calc = [](double v) {
        v /= 255.;
        if (v <= LRGB_CUTOFF) {
            return v / 12.92;
        }
        return cmath::pow((v + 0.055) / 1.055, 2.4);
    };

    if (value >= 0) {
        return calc(value);
    }
    return -calc(-value);
}

constexpr auto to_rgb(double value) -> double {
    const auto calc = [](double v) {
        if (v <= RGB_CUTOFF) {
            return 12.92 * v * 255.;
        }
        return (1.055 * cmath::pow(v, 1. / 2.4) - 0.055) * 255.;
    };

    if (value >= 0) {
        return calc(value);
    }
    return -calc(-value);
}

}  // namespace details::ct

constexpr auto to_lrgb(Rgb c) -> Lrgb {
    return Lrgb {
        .r = details::ct::to_lrgb(c.r),
        .g = details::ct::to_lrgb(c.g),
        .b = details::ct::to_lrgb(c.b),
    };
}

constexpr auto to_rgb(Lrgb c) -> Rgb {
    return Rgb {
        .r = details::ct::to_rgb(c.r),
        .g = details::ct::to_rgb(c.g),
        .b = details::ct::to_rgb(c.b),
    };
}

/*
 * M1 and M2 are specified as in:
 *
 * https://bottosson.github.io/posts/oklab/#converting-from-linear-srgb-to-oklab
 */

constexpr auto M1 = std::array {
    std::array {0.4122214708, 0.5363325363, 0.0514459929},
    std::array {0.2119034982, 0.6806995451, 0.1073969566},
    std::array {0.0883024619, 0.2817188376, 0.6299787005},
};

constexpr auto M2 = std::array {
    std::array {0.2104542553, 0.7936177850, -0.0040720468},
    std::array {1.9779984951, -2.4285922050, 0.4505937099},
    std::array {0.0259040371, 0.7827717662, -0.8086757660},
};

/*
 * Inverse matrix values are calculated from M1 and M2 to 50 digits.
 *
 * For calculation python mpmath is used:
 *
 *    from mpmath import mp
 *    mp.dps = 50
 *
 *    M1 = mp.matrix([
 *      ['0.4122214708', '0.5363325363', '0.0514459929'],
 *      ['0.2119034982', '0.6806995451', '0.1073969566'],
 *      ['0.0883024619', '0.2817188376', '0.6299787005']
 *    ])
 *
 *    M2 = mp.matrix([
 *      ['0.2104542553', ' 0.7936177850', '-0.0040720468'],
 *      ['1.9779984951', '-2.4285922050', ' 0.4505937099'],
 *      ['0.0259040371', ' 0.7827717662', '-0.8086757660']
 *    ])
 *
 *    print(M1**-1)
 *    print()
 *    print(M2**-1)
 */

constexpr auto M1i = std::array {
    std::array {
        4.0767416613479942676681908333711298900607278264432,
        -3.3077115904081933131586607842489318886561825334207,
        0.23096992872942788644965061956193592017056151811188,
    },
    std::array {
        -1.2684380040921760691815055595117506020901414005992,
        2.6097574006633714302405009528423362305619233855269,
        -0.34131939631021962099265825030653553318754836187154,
    },
    std::array {
        -0.0041960865418371092973767821251846315637521173373797,
        -0.70341861445944960601310996913659932654899822384405,
        1.7076147009309448538645417906604729611990904085265,
    },
};

constexpr auto M2i = std::array {
    std::array {
        0.99999999845051981426207542502031373637162589278552,
        0.39633779217376785682345989261573192476766903603149,
        0.21580375806075880342314146183003789259061778746721,
    },
    std::array {
        1.0000000088817607767160752456704707127618367741013,
        -0.10556134232365634941095687705472233997368274023635,
        -0.063854174771705903405254198817795633810975771081532,
    },
    std::array {
        1.0000000546724109177012928651534461072184102869894,
        -0.089484182094965759689052745863391341306696697157345,
        -1.2914855378640917399489287529147724018785456753707,
    },
};

constexpr auto to_oklab(Lrgb c) -> Oklab {
    const auto l = M1[0][0] * c.r + M1[0][1] * c.g + M1[0][2] * c.b;
    const auto m = M1[1][0] * c.r + M1[1][1] * c.g + M1[1][2] * c.b;
    const auto s = M1[2][0] * c.r + M1[2][1] * c.g + M1[2][2] * c.b;

    const auto l_ = cmath::cbrt(l);
    const auto m_ = cmath::cbrt(m);
    const auto s_ = cmath::cbrt(s);

    return Oklab {
        .l = M2[0][0] * l_ + M2[0][1] * m_ + M2[0][2] * s_,
        .a = M2[1][0] * l_ + M2[1][1] * m_ + M2[1][2] * s_,
        .b = M2[2][0] * l_ + M2[2][1] * m_ + M2[2][2] * s_,
    };
}

constexpr auto to_lrgb(Oklab c) -> Lrgb {
    const auto l_ = M2i[0][0] * c.l + M2i[0][1] * c.a + M2i[0][2] * c.b;
    const auto m_ = M2i[1][0] * c.l + M2i[1][1] * c.a + M2i[1][2] * c.b;
    const auto s_ = M2i[2][0] * c.l + M2i[2][1] * c.a + M2i[2][2] * c.b;

    const auto l = l_ * l_ * l_;
    const auto m = m_ * m_ * m_;
    const auto s = s_ * s_ * s_;

    return Lrgb {
        .r = M1i[0][0] * l + M1i[0][1] * m + M1i[0][2] * s,
        .g = M1i[1][0] * l + M1i[1][1] * m + M1i[1][2] * s,
        .b = M1i[2][0] * l + M1i[2][1] * m + M1i[2][2] * s,
    };
}

constexpr auto to_oklch(Oklab c) -> Oklch {
    constexpr auto rad_to_deg = 180. / std::numbers::pi;
    const auto chroma = cmath::hypot(c.a, c.b);

    const auto hue = [&] {
        if (chroma < defaults::ct::min_valid_chroma) {
            return 0.;
        }

        auto hue = cmath::atan2(c.b, c.a) * rad_to_deg;
        if (hue < 0) {
            hue += 360.;
        }
        return hue;
    }();

    return Oklch {
        .l = c.l,
        .c = chroma,
        .h = hue,
    };
}

constexpr auto to_oklab(Oklch lch) -> Oklab {
    constexpr auto deg_to_rad = std::numbers::pi / 180.;

    const auto chroma = std::max(0., lch.c);
    return Oklab {
        .l = lch.l,
        .a = chroma * cmath::cos(lch.h * deg_to_rad),
        .b = chroma * cmath::sin(lch.h * deg_to_rad),
    };
}

namespace details::ct {

/*
 * Normalized a and b, so a^2 + b^2 == 1
 */
class ab_norm_t {
   public:
    constexpr explicit ab_norm_t(Oklab lab) noexcept {
        const auto c = cmath::hypot(lab.a, lab.b);

        c_ = c;
        if (!cmath::isfinite(c) || c < defaults::ct::min_valid_chroma) {
            a_norm_ = 1.;
            b_norm_ = 0.;
        } else {
            a_norm_ = lab.a / c;
            b_norm_ = lab.b / c;
        }
    }

    [[nodiscard]] constexpr auto operator==(const ab_norm_t&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto c() const noexcept -> double {
        return c_;
    }

    [[nodiscard]] constexpr auto a_norm() const noexcept -> double {
        return a_norm_;
    }

    [[nodiscard]] constexpr auto b_norm() const noexcept -> double {
        return b_norm_;
    }

   private:
    double c_;
    double a_norm_;
    double b_norm_;
};

[[nodiscard]] constexpr auto is_lrgb_representable_srgb(double value) -> bool {
    constexpr auto low = 0. - defaults::ct::lrgb_is_valid_srgb_epsilon;
    constexpr auto high = 1. + defaults::ct::lrgb_is_valid_srgb_epsilon;

    return value >= low && value <= high;
}

[[nodiscard]] constexpr auto is_representable_srgb(const Lrgb& lrgb) -> bool {
    return is_lrgb_representable_srgb(lrgb.r) &&  //
           is_lrgb_representable_srgb(lrgb.g) &&  //
           is_lrgb_representable_srgb(lrgb.b);
}

template <int k>
[[nodiscard]] constexpr auto is_representable_srgb_k(const Lrgb& lrgb) -> bool {
    auto v_ = double {};

    if constexpr (k == 0) {
        v_ = lrgb.r;
    } else if constexpr (k == 1) {
        v_ = lrgb.g;
    } else if constexpr (k == 2) {
        v_ = lrgb.b;
    } else {
        static_assert(false);
    }

    return is_lrgb_representable_srgb(v_);
}

[[nodiscard]] constexpr auto is_representable_srgb(Oklab lab) -> bool {
    return is_representable_srgb(to_lrgb(lab));
}

}  // namespace details::ct

namespace defaults {

constexpr static auto dark_mode_rgbi [[maybe_unused]] = RgbI {
    .r = dark_mode_gray,
    .g = dark_mode_gray,
    .b = dark_mode_gray,
};
constexpr static auto dark_mode_rgb = Rgb {
    .r = dark_mode_gray,
    .g = dark_mode_gray,
    .b = dark_mode_gray,
};
constexpr static auto dark_mode_oklab = to_oklab(to_lrgb(dark_mode_rgb));

}  // namespace defaults

namespace details::ct {

[[nodiscard]] constexpr auto is_angle_down_zero_chroma(double l) -> bool {
    constexpr auto eps = defaults::ct::luminance_zone_with_zero_chroma;

    return l < eps || l > (1. - eps);
};

[[nodiscard]] constexpr auto get_radius_down(Oklab lab) -> double {
    const auto l_length = 1. - lab.l;

    if (l_length <= 0.) {
        return 0.;
    };

    if (is_angle_down_zero_chroma(lab.l)) {
        return l_length;
    };

    return cmath::hypot(l_length, lab.a, lab.b);
}

[[nodiscard]] constexpr auto get_angle_down(Oklab lab) -> double {
    if (is_angle_down_zero_chroma(lab.l)) {
        return 0.;
    };

    const auto c = cmath::hypot(lab.a, lab.b);
    return cmath::atan(c / (1. - lab.l));
}

[[nodiscard]] constexpr auto from_angle_down(double l_radius, ab_norm_t ab, double beta)
    -> Oklab {
    const auto c = l_radius * cmath::sin(beta);
    Expects(c >= 0);

    return Oklab {
        .l = 1. - l_radius * cmath::cos(beta),
        .a = c * ab.a_norm(),
        .b = c * ab.b_norm(),
    };
};

[[nodiscard]] constexpr auto is_angle_up_zero_chroma(double l) -> bool {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;
    constexpr auto eps = (1. - l_dark) * defaults::ct::luminance_zone_with_zero_chroma;

    return l < l_dark + eps || l > (1. - eps);
};

[[nodiscard]] constexpr auto get_radius_up(Oklab lab) -> double {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    const auto l_length = lab.l - l_dark;

    if (l_length <= 0.) {
        return 0.;
    };

    if (is_angle_up_zero_chroma(lab.l)) {
        return l_length;
    };

    return cmath::hypot(l_length, lab.a, lab.b);
}

[[nodiscard]] constexpr auto get_angle_up(Oklab lab) -> double {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    if (is_angle_up_zero_chroma(lab.l)) {
        return 0.;
    };

    const auto c = cmath::hypot(lab.a, lab.b);
    return cmath::atan(c / (lab.l - l_dark));
}

[[nodiscard]] constexpr auto from_angle_up(double l_radius, ab_norm_t ab, double beta)
    -> Oklab {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    const auto c = l_radius * cmath::sin(beta);
    Expects(c >= 0);

    return Oklab {
        .l = l_dark + l_radius * cmath::cos(beta),
        .a = c * ab.a_norm(),
        .b = c * ab.b_norm(),
    };
}

struct AngleColor {
    double beta;
    Lrgb color;

    [[nodiscard]] constexpr auto operator==(const AngleColor&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

using AngleBracket = std::pair<AngleColor, AngleColor>;
using AngleBracketPair = std::pair<AngleBracket, AngleBracket>;

// struct AngleBracketPair {
// [[nodiscard]]     constexpr explicit auto AngleBracketPair(AngleColorBracket
// }

template <typename Func>
concept FuncAngleToLrgb = std::invocable<Func, double> &&
                          std::same_as<std::invoke_result_t<Func, double>, Lrgb>;

template <int k, FuncAngleToLrgb Func>
[[nodiscard]] constexpr auto find_max_circle_angle_split(const AngleBracket& p,
                                                         Func angle_to_lrgb)
    -> AngleBracketPair {
    const auto is_srgb = is_representable_srgb_k<k>;

    const auto [a, b] = p;
    if (a.beta >= b.beta) {
        throw std::runtime_error {"first angle in bracket need to be smaller"};
    }

    const auto a_srgb = is_srgb(a.color);
    const auto b_srgb = is_srgb(b.color);

    if (a_srgb == b_srgb) {
        throw std::runtime_error {"unable to split as there is no sign change"};
    }

    // bracket search
    auto low_srgb = a_srgb;
    auto low = a;
    auto high = b;
    Expects(low.beta < high.beta);

    for (auto i = 0; i < defaults::ct::beta_refinement_steps; ++i) {
        const auto mid_beta = (low.beta + high.beta) / 2.;

        if (mid_beta == low.beta || mid_beta == high.beta) {
            break;
        }

        const auto mid = AngleColor {
            .beta = mid_beta,
            .color = angle_to_lrgb(mid_beta),
        };

        if (is_srgb(mid.color) == low_srgb) {
            low = mid;
        } else {
            high = mid;
        }
    };

    const auto result = std::pair {
        AngleBracket {a, low},
        AngleBracket {high, b},
    };
    Ensures(is_srgb(result.first.first.color) == is_srgb(result.first.second.color));
    Ensures(is_srgb(result.second.first.color) == is_srgb(result.second.second.color));
    Ensures(result.first.first.beta <= result.first.second.beta);
    Ensures(result.first.second.beta <= result.second.first.beta);
    Ensures(result.second.first.beta <= result.second.second.beta);
    return result;
}

template <FuncAngleToLrgb Func>
[[nodiscard]] constexpr auto find_max_circle_angle_bracket(const AngleBracket& p,
                                                           Func angle_to_lrgb)
    -> std::optional<double> {
    const auto [a, b] = p;
    if (a.beta >= b.beta) {
        throw std::runtime_error {"first angle in bracket need to be smaller"};
    }

    constexpr auto is_srgb = is_lrgb_representable_srgb;

    const auto flip_r = is_srgb(a.color.r) != is_srgb(b.color.r);
    const auto flip_g = is_srgb(a.color.g) != is_srgb(b.color.g);
    const auto flip_b = is_srgb(a.color.b) != is_srgb(b.color.b);

    const auto valid_r = flip_r || (is_srgb(a.color.r) && is_srgb(b.color.r));
    const auto valid_g = flip_g || (is_srgb(a.color.g) && is_srgb(b.color.g));
    const auto valid_b = flip_b || (is_srgb(a.color.b) && is_srgb(b.color.b));

    const auto has_split = (flip_r || flip_g || flip_b) && valid_r && valid_g && valid_b;
    if (!has_split) {
        return std::nullopt;
    }

    // perform split on first component that has sign change
    const auto splits = [&]() constexpr -> AngleBracketPair {
        if (flip_r) {
            return find_max_circle_angle_split<0>(p, angle_to_lrgb);
        }
        if (flip_g) {
            return find_max_circle_angle_split<1>(p, angle_to_lrgb);
        }
        Expects(flip_b);
        return find_max_circle_angle_split<2>(p, angle_to_lrgb);
    }();

    // check upper
    if (const auto upper = find_max_circle_angle_bracket(splits.first, angle_to_lrgb)) {
        return upper;
    }

    // check middle
    if (is_representable_srgb(splits.second.first.color)) {
        return splits.second.first.beta;
    }
    if (is_representable_srgb(splits.first.second.color)) {
        return splits.first.second.beta;
    }

    // check lower
    return find_max_circle_angle_bracket(splits.second, angle_to_lrgb);
}

struct MaxAngleConfig {
    int beta_count;
    double beta_max_rad;
};

[[nodiscard]] constexpr auto to_beta_step(const MaxAngleConfig& config) -> double {
    Expects(config.beta_count >= 2);
    Expects(config.beta_max_rad >= 0.);
    return config.beta_max_rad / (config.beta_count - 1);
}

struct ValidMaxAngleConfig {
   public:
    [[nodiscard]] explicit constexpr ValidMaxAngleConfig(MaxAngleConfig config)
        : beta_count_ {config.beta_count},
          beta_max_ {config.beta_max_rad},
          beta_step_ {to_beta_step(config)} {}

    [[nodiscard]] constexpr auto beta_count() const noexcept -> int {
        return beta_count_;
    }

    [[nodiscard]] constexpr auto beta_max() const noexcept -> double {
        return beta_max_;
    }

    [[nodiscard]] constexpr auto beta_step() const noexcept -> double {
        return beta_step_;
    }

   private:
    int beta_count_;
    double beta_max_;
    double beta_step_;
};

template <FuncAngleToLrgb Func>
[[nodiscard]] constexpr auto find_max_circle_angle(ValidMaxAngleConfig config,
                                                   Func angle_to_lrgb) -> double {
    const auto to_beta = [&](int i_) constexpr -> double {  //
        return config.beta_step() * i_;
    };
    const auto to_color = [&](double beta_) constexpr -> AngleColor {
        return AngleColor {
            .beta = beta_,
            .color = angle_to_lrgb(beta_),
        };
    };

    // cicle is fully inside sRGB gammut
    if (is_representable_srgb(to_color(config.beta_max()).color)) {
        return config.beta_max();
    }

    auto v = std::views::iota(0, config.beta_count()) |  //
             std::views::transform(to_beta) |            //
             std::views::transform(to_color) |           //
             std::views::pairwise |                      //
             std::views::reverse;

    for (const auto& bracket : v) {
        const auto [a, b] = bracket;
        if (const auto beta =
                find_max_circle_angle_bracket(std::pair {a, b}, angle_to_lrgb)) {
            Ensures(is_representable_srgb(angle_to_lrgb(beta.value())));
            return beta.value();
        }
    }

    return 0.;
}

/*
 * @brief: Find maximum angle down with valid sRGB value.
 *
 * Angle down is convex for e.g. RGB(0, 0, 250). To find multiple roots where the
 * circle intersects the sRGB surface, first brackets which contain them are searched.
 * Then each bracked is solved for the exact transition point.
 */
[[nodiscard]] constexpr auto max_circle_angle_down_slow(double l_radius, ab_norm_t ab)
    -> double {
    constexpr auto deg_to_rad = std::numbers::pi / 180.;

    if (is_angle_down_zero_chroma(1. - l_radius)) {
        return 0.;
    }

    const auto angle_to_lrgb = [&](double beta_) constexpr -> Lrgb {
        return to_lrgb(from_angle_down(l_radius, ab, beta_));
    };

    constexpr auto config = ValidMaxAngleConfig {MaxAngleConfig {
        .beta_count = defaults::ct::beta_down_count,
        .beta_max_rad = defaults::ct::beta_down_max_deg * deg_to_rad,
    }};
    return find_max_circle_angle(config, angle_to_lrgb);
}

/*
 * @brief: Find maximum angle up with valid sRGB value.
 */
[[nodiscard]] constexpr auto max_circle_angle_up_slow(double l_radius, ab_norm_t ab)
    -> double {
    constexpr auto deg_to_rad = std::numbers::pi / 180.;
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    if (is_angle_up_zero_chroma(l_dark + l_radius)) {
        return 0.;
    }

    const auto angle_to_lrgb = [&](double beta_) constexpr -> Lrgb {
        return to_lrgb(from_angle_up(l_radius, ab, beta_));
    };

    constexpr auto config = ValidMaxAngleConfig {MaxAngleConfig {
        .beta_count = defaults::ct::beta_up_count,
        .beta_max_rad = defaults::ct::beta_up_max_deg * deg_to_rad,
    }};
    return find_max_circle_angle(config, angle_to_lrgb);
}

[[nodiscard]] constexpr auto to_dark_mode_raw(Rgb rgb) -> Oklab {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    const auto lab = to_oklab(to_lrgb(rgb));
    const auto ab = ab_norm_t {lab};

    // distance and angles
    const auto r_light = get_radius_down(lab);
    const auto r_dark = r_light * (1. - l_dark);

    const auto b_light_max = max_circle_angle_down_slow(r_light, ab);
    const auto b_dark_max = max_circle_angle_up_slow(r_dark, ab);

    const auto b_light = get_angle_down(lab);
    const auto b_light_ratio_raw =
        b_light / std::max(b_light_max, defaults::ct::min_max_chroma_angle_rad);
    const auto b_light_ratio = cmath::clamp(b_light_ratio_raw, 0., 1.);

    // derive
    const auto b_dark_ratio = b_light_ratio;
    const auto b_dark = b_dark_max * b_dark_ratio;
    return from_angle_up(r_dark, ab, b_dark);
}

[[nodiscard]] constexpr auto to_light_mode_raw(Rgb rgb) -> Oklab {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    const auto lab = to_oklab(to_lrgb(rgb));
    const auto ab = ab_norm_t {lab};

    // distance and angles
    const auto r_dark = get_radius_up(lab);
    const auto r_light = r_dark * (1. / (1. - l_dark));

    const auto b_dark_max = max_circle_angle_up_slow(r_dark, ab);
    const auto b_light_max = max_circle_angle_down_slow(r_light, ab);

    const auto b_dark = get_angle_up(lab);
    const auto b_dark_ratio_raw =
        b_dark / std::max(b_dark_max, defaults::ct::min_max_chroma_angle_rad);
    const auto b_dark_ratio = cmath::clamp(b_dark_ratio_raw, 0., 1.);

    // derive
    const auto b_light_ratio = b_dark_ratio;
    const auto b_light = b_light_max * b_light_ratio;
    return from_angle_down(r_light, ab, b_light);
}

/*
 * @brief: Convert to light / dark & find best rounding.
 */
template <bool to_dark>
constexpr auto to_mode_rounding(RgbI rgb) -> RgbI {
    const auto input = Rgb {
        .r = static_cast<double>(rgb.r),
        .g = static_cast<double>(rgb.g),
        .b = static_cast<double>(rgb.b),
    };
    const auto input_oklab = to_oklab(to_lrgb(input));
    const auto raw_oklab = to_dark ? details::ct::to_dark_mode_raw(input)
                                   : details::ct::to_light_mode_raw(input);
    const auto raw = to_rgb(to_lrgb(raw_oklab));

    // create all possible roundings
    const auto floor = [](double v_) constexpr -> double {
        return cmath::clamp(cmath::floor(v_), 0., 255.);
    };
    const auto ceil = [](double v_) constexpr -> double {
        return cmath::clamp(cmath::ceil(v_), 0., 255.);
    };
    auto candidates = static_vector<Rgb, 8> {
        Rgb {.r = ceil(raw.r), .g = ceil(raw.g), .b = ceil(raw.b)},
        Rgb {.r = floor(raw.r), .g = ceil(raw.g), .b = ceil(raw.b)},
        Rgb {.r = ceil(raw.r), .g = floor(raw.g), .b = ceil(raw.b)},
        Rgb {.r = floor(raw.r), .g = floor(raw.g), .b = ceil(raw.b)},
        Rgb {.r = ceil(raw.r), .g = ceil(raw.g), .b = floor(raw.b)},
        Rgb {.r = floor(raw.r), .g = ceil(raw.g), .b = floor(raw.b)},
        Rgb {.r = ceil(raw.r), .g = floor(raw.g), .b = floor(raw.b)},
        Rgb {.r = floor(raw.r), .g = floor(raw.g), .b = floor(raw.b)},
    };

    // remove deplucicate (happens when clamped)
    std::ranges::sort(candidates);
    candidates.erase(std::ranges::unique(candidates).begin(), candidates.end());

    // calculate distance converting back each candidate
    const auto to_distance = [&](Rgb c_) constexpr -> double {
        const auto back_ = to_dark ? details::ct::to_light_mode_raw(c_)
                                   : details::ct::to_dark_mode_raw(c_);
        return distance(input_oklab, back_);
    };
    const auto distances = candidates |                          //
                           std::views::transform(to_distance) |  //
                           std::ranges::to<static_vector<double, 8>>();

    // select best candidate
    Expects(!distances.empty());
    const auto min_idx = std::ranges::min_element(distances) - distances.begin();
    const auto best = candidates.at(min_idx);

    return to_rgbi(best);
}

}  // namespace details::ct

constexpr auto to_dark_mode(RgbI rgb) -> RgbI {
    if (rgb == RgbI {.r = 255, .g = 255, .b = 255}) {
        return defaults::dark_mode_rgbi;
    }
    return details::ct::to_mode_rounding<true>(rgb);
}

constexpr auto to_light_mode(RgbI rgb) -> RgbI {
    return details::ct::to_mode_rounding<false>(rgb);
}

}  // namespace logicsim

#endif
