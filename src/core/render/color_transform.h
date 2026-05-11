#ifndef LOGICSIM_CORE_RENDER_COLOR_TRANSFORM_H
#define LOGICSIM_CORE_RENDER_COLOR_TRANSFORM_H

#include "core/format/struct.h"
#include "core/logging.h"

#include <gcem.hpp>

#include <cmath>
#include <limits>
#include <numbers>

/*
 * @brief: Define oklab color transformations.
 *
 * Source: https://bottosson.github.io/posts/gamutclipping/
 * Source: https://en.wikipedia.org/wiki/SRGB
 */

namespace logicsim {

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

[[nodiscard]] constexpr auto is_close(const Oklch& x, const Oklch& y) -> bool;

[[nodiscard]] constexpr auto to_lrgb(Rgb c) -> Lrgb;
[[nodiscard]] constexpr auto to_rgb(Lrgb c) -> Rgb;

[[nodiscard]] constexpr auto to_oklab(Lrgb c) -> Oklab;
[[nodiscard]] constexpr auto to_lrgb(Oklab c) -> Lrgb;

[[nodiscard]] constexpr auto to_oklch(Oklab c) -> Oklch;
[[nodiscard]] constexpr auto to_oklab(Oklch lch) -> Oklab;

[[nodiscard]] constexpr auto to_dark_mode(Rgb rgb) -> Rgb;
[[nodiscard]] constexpr auto to_light_mode(Rgb rgb) -> Rgb;

//
// Implementation
//

namespace details::ct {

[[nodiscard]] constexpr auto abs(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::abs(x);
    }
    return std::abs(x);
}

[[nodiscard]] constexpr auto pow(double x, double y) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::pow(x, y);
    }
    return std::pow(x, y);
}

[[nodiscard]] constexpr auto cbrt(double x) -> double {
    if (std::is_constant_evaluated()) {
        if (x == 0) {
            return 0;
        }
        // use double to get full double precision
        if (x < 0) {
            return -gcem::pow(-x, 1.0 / 3.0);
        }
        return gcem::pow(x, 1.0 / 3.0);
    }
    return std::cbrt(x);
}

[[nodiscard]] constexpr auto sqrt(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::sqrt(x);
    }
    return std::sqrt(x);
}

[[nodiscard]] constexpr auto hypot(double x, double y) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::hypot(x, y);
    }
    return std::hypot(x, y);
}

[[nodiscard]] constexpr auto hypot(double x, double y, double z) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::sqrt(x * x + y * y + z * z);
    }
    return std::hypot(x, y, z);
}

[[nodiscard]] constexpr auto sin(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::sin(x);
    }
    return std::sin(x);
}

[[nodiscard]] constexpr auto cos(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::cos(x);
    }
    return std::cos(x);
}

[[nodiscard]] constexpr auto atan(double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::atan(x);
    }
    return std::atan(x);
}

[[nodiscard]] constexpr auto atan2(double y, double x) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::atan2(y, x);
    }
    return std::atan2(y, x);
}

[[nodiscard]] constexpr auto fmod(double x, double y) -> double {
    if (std::is_constant_evaluated()) {
        return gcem::fmod(x, y);
    }
    return std::fmod(x, y);
}

[[nodiscard]] constexpr auto clamp(double x, double min, double max) -> double {
    if (x < min) {
        return min;
    }
    if (x > max) {
        return max;
    }

    return x;
}

[[nodiscard]] constexpr auto sgn(double x) -> double {
    return static_cast<double>(0 < x) - static_cast<double>(x < 0);
}

}  // namespace details::ct

constexpr auto is_close(const Rgb& x, const Rgb& y) -> bool {
    constexpr auto tol = 255.0 * 1e-14;
    return details::ct::abs(x.r - y.r) < tol &&  //
           details::ct::abs(x.g - y.g) < tol &&  //
           details::ct::abs(x.b - y.b) < tol;
}

constexpr auto is_close(const Lrgb& x, const Lrgb& y) -> bool {
    constexpr auto tol = 1e-14;
    return details::ct::abs(x.r - y.r) < tol &&  //
           details::ct::abs(x.g - y.g) < tol &&  //
           details::ct::abs(x.b - y.b) < tol;
}

constexpr auto is_close(const Oklab& x, const Oklab& y) -> bool {
    constexpr auto tol = 1e-14;
    return details::ct::abs(x.l - y.l) < tol &&  //
           details::ct::abs(x.a - y.a) < tol &&  //
           details::ct::abs(x.b - y.b) < tol;
}

constexpr auto is_close(const Oklch& x, const Oklch& y) -> bool {
    constexpr auto tol_lc = 1e-14;
    constexpr auto tol_h = 360. * 1e-14;

    if (details::ct::abs(x.l - y.l) >= tol_lc) {
        return false;
    }
    if (details::ct::abs(x.c - y.c) >= tol_lc) {
        return false;
    }
    if (details::ct::abs(x.c) < tol_lc &&  //
        details::ct::abs(y.c) < tol_lc) {
        return true;
    }

    const auto diff = details::ct::abs(details::ct::fmod(x.h - y.h, 360.));
    return (diff < tol_h) || (360. - diff < tol_h);
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
        return details::ct::pow((v + 0.055) / 1.055, 2.4);
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
        return (1.055 * details::ct::pow(v, 1. / 2.4) - 0.055) * 255.;
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

    const auto l_ = details::ct::cbrt(l);
    const auto m_ = details::ct::cbrt(m);
    const auto s_ = details::ct::cbrt(s);

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
    const auto chroma = details::ct::hypot(c.a, c.b);

    const auto hue = [&] {
        if (chroma < 1.0e-6) {
            return 0.;
        }

        auto hue = details::ct::atan2(c.b, c.a) * rad_to_deg;
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
        .a = chroma * details::ct::cos(lch.h * deg_to_rad),
        .b = chroma * details::ct::sin(lch.h * deg_to_rad),
    };
}

namespace details::ct {

/*
 * Normalized a and b, so a^2 + b^2 == 1
 */
class ab_norm_t {
   public:
    constexpr explicit ab_norm_t(Oklab lab) noexcept {
        constexpr auto eps = 1e-5;

        const auto c = details::ct::hypot(lab.a, lab.b);

        c_ = c;
        if (!std::isfinite(c) || c < eps) {
            a_norm_ = 1.;
            b_norm_ = 0.;
        } else {
            a_norm_ = lab.a / c;
            b_norm_ = lab.b / c;
        }
    }

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

/*
 * Finds the maximum saturation possible for a given hue that fits in sRGB
 *
 * Saturation here is defined as S = C/L
 * a and b must be normalized so a^2 + b^2 == 1
 *
 * Max saturation will be when one of r, g or b goes below zero.
 */
[[nodiscard]] constexpr auto compute_max_saturation(ab_norm_t ab) -> double {
    const auto a = ab.a_norm();
    const auto b = ab.b_norm();

    // Select different coefficients depending on which component goes below zero
    // first
    double k0 = {};
    double k1 = {};
    double k2 = {};
    double k3 = {};
    double k4 = {};
    double wl = {};
    double wm = {};
    double ws = {};

    if (-1.88170328 * a - 0.80936493 * b > 1) {
        // Red component
        k0 = +1.19086277;
        k1 = +1.76576728;
        k2 = +0.59662641;
        k3 = +0.75515197;
        k4 = +0.56771245;
        wl = +4.0767416621;
        wm = -3.3077115913;
        ws = +0.2309699292;
    } else if (1.81444104 * a - 1.19445276 * b > 1) {
        // Green component
        k0 = +0.73956515;
        k1 = -0.45954404;
        k2 = +0.08285427;
        k3 = +0.12541070;
        k4 = +0.14503204;
        wl = -1.2684380046;
        wm = +2.6097574011;
        ws = -0.3413193965;
    } else {
        // Blue component
        k0 = +1.35733652;
        k1 = -0.00915799;
        k2 = -1.15130210;
        k3 = -0.50559606;
        k4 = +0.00692167;
        wl = -0.0041960863;
        wm = -0.7034186147;
        ws = +1.7076147010;
    }

    // Approximate max saturation using a polynomial:
    const auto S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

    // Do one step Halley's method to get closer
    // this gives an error less than 10e6, except for some blue hues where the dS/dh
    // is close to infinite this should be sufficient for most applications, otherwise
    // do two/three steps

    const auto k_l = +0.3963377774 * a + 0.2158037573 * b;
    const auto k_m = -0.1055613458 * a - 0.0638541728 * b;
    const auto k_s = -0.0894841775 * a - 1.2914855480 * b;

    {
        const auto l_ = 1. + S * k_l;
        const auto m_ = 1. + S * k_m;
        const auto s_ = 1. + S * k_s;

        const auto l = l_ * l_ * l_;
        const auto m = m_ * m_ * m_;
        const auto s = s_ * s_ * s_;

        const auto l_dS = 3. * k_l * l_ * l_;
        const auto m_dS = 3. * k_m * m_ * m_;
        const auto s_dS = 3. * k_s * s_ * s_;

        const auto l_dS2 = 6. * k_l * k_l * l_;
        const auto m_dS2 = 6. * k_m * k_m * m_;
        const auto s_dS2 = 6. * k_s * k_s * s_;

        const auto f = wl * l + wm * m + ws * s;
        const auto f1 = wl * l_dS + wm * m_dS + ws * s_dS;
        const auto f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

        return S - f * f1 / (f1 * f1 - 0.5 * f * f2);
    }
}

struct LC {
    double L;
    double C;
};

/*
 * @brief: finds L_cusp and C_cusp for a given hue
 *
 * a and b must be normalized so a^2 + b^2 == 1
 */

[[nodiscard]] constexpr auto find_cusp(ab_norm_t ab) -> LC {
    // First, find the maximum saturation (saturation S = C/L)
    const auto S_cusp = compute_max_saturation(ab);

    // Convert to linear sRGB to find the first point where at least one of r,g or b
    // >= 1:
    const Lrgb rgb_at_max = to_lrgb(Oklab {
        .l = 1,
        .a = S_cusp * ab.a_norm(),
        .b = S_cusp * ab.b_norm(),
    });
    const auto L_cusp =
        details::ct::cbrt(1. / std::max({rgb_at_max.r, rgb_at_max.g, rgb_at_max.b}));
    const auto C_cusp = L_cusp * S_cusp;

    return LC {
        .L = L_cusp,
        .C = C_cusp,
    };
}

/*
 * @brief: Finds intersection of the line defined by
 *
 * L = L0 * (1 - t) + t * L1;
 * C = t * C1;
 *
 * a and b must be normalized so a^2 + b^2 == 1
 */
[[nodiscard]] constexpr auto find_gamut_intersection(ab_norm_t ab, double L1, double C1,
                                                     double L0) -> double {
    constexpr auto flt_max = std::numeric_limits<double>::max();
    const auto a = ab.a_norm();
    const auto b = ab.b_norm();

    // Find the cusp of the gamut triangle
    const auto cusp = find_cusp(ab);

    // Find the intersection for upper and lower half seprately
    double t = {};
    if (((L1 - L0) * cusp.C - (cusp.L - L0) * C1) <= 0.) {
        // Lower half

        t = cusp.C * L0 / (C1 * cusp.L + cusp.C * (L0 - L1));
    } else {
        // Upper half

        // First intersect with triangle
        t = cusp.C * (L0 - 1.) / (C1 * (cusp.L - 1.) + cusp.C * (L0 - L1));

        // Then one step Halley's method
        {
            double dL = L1 - L0;
            double dC = C1;

            double k_l = +0.3963377774 * a + 0.2158037573 * b;
            double k_m = -0.1055613458 * a - 0.0638541728 * b;
            double k_s = -0.0894841775 * a - 1.2914855480 * b;

            double l_dt = dL + dC * k_l;
            double m_dt = dL + dC * k_m;
            double s_dt = dL + dC * k_s;

            // If higher accuracy is required, 2 or 3 iterations of the following
            // block can be used:
            {
                double L = L0 * (1. - t) + t * L1;
                double C = t * C1;

                double l_ = L + C * k_l;
                double m_ = L + C * k_m;
                double s_ = L + C * k_s;

                double l = l_ * l_ * l_;
                double m = m_ * m_ * m_;
                double s = s_ * s_ * s_;

                double ldt = 3 * l_dt * l_ * l_;
                double mdt = 3 * m_dt * m_ * m_;
                double sdt = 3 * s_dt * s_ * s_;

                double ldt2 = 6 * l_dt * l_dt * l_;
                double mdt2 = 6 * m_dt * m_dt * m_;
                double sdt2 = 6 * s_dt * s_dt * s_;

                double r = 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s - 1;
                double r1 = 4.0767416621 * ldt - 3.3077115913 * mdt + 0.2309699292 * sdt;
                double r2 =
                    4.0767416621 * ldt2 - 3.3077115913 * mdt2 + 0.2309699292 * sdt2;

                double u_r = r1 / (r1 * r1 - 0.5 * r * r2);
                double t_r = -r * u_r;

                double g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s - 1;
                double g1 = -1.2684380046 * ldt + 2.6097574011 * mdt - 0.3413193965 * sdt;
                double g2 =
                    -1.2684380046 * ldt2 + 2.6097574011 * mdt2 - 0.3413193965 * sdt2;

                double u_g = g1 / (g1 * g1 - 0. * g * g2);
                double t_g = -g * u_g;

                double b_ = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s - 1;
                double b1 = -0.0041960863 * ldt - 0.7034186147 * mdt + 1.7076147010 * sdt;
                double b2 =
                    -0.0041960863 * ldt2 - 0.7034186147 * mdt2 + 1.7076147010 * sdt2;

                double u_b = b1 / (b1 * b1 - 0.5 * b_ * b2);
                double t_b = -b_ * u_b;

                t_r = u_r >= 0. ? t_r : flt_max;
                t_g = u_g >= 0. ? t_g : flt_max;
                t_b = u_b >= 0. ? t_b : flt_max;

                t += std::min({t_r, t_g, t_b});
            }
        }
    }

    return t;
}

[[nodiscard]] constexpr auto gamut_clip_preserve_chroma(Lrgb rgb) -> Lrgb {
    if (rgb.r <= 1. && rgb.g <= 1. && rgb.b <= 1. &&  //
        rgb.r >= 0. && rgb.g >= 0. && rgb.b >= 0.) {
        return rgb;
    }

    const auto lab = to_oklab(rgb);

    const auto L = lab.l;
    const auto ab = ab_norm_t {lab};
    const auto C = ab.c();
    /*
    const auto L = lab.l;
    const auto eps = 0.00001;
    const auto C = std::max(eps, details::ct::sqrtf(lab.a * lab.a + lab.b * lab.b));
    const auto a_ = lab.a / C;
    const auto b_ = lab.b / C;
    */

    const auto L0 = details::ct::clamp(L, 0., 1.);

    const auto t = find_gamut_intersection(ab, L, C, L0);
    const auto L_clipped = L0 * (1. - t) + t * L;
    const auto C_clipped = t * C;

    const auto res = to_lrgb(Oklab {
        .l = L_clipped,
        .a = C_clipped * ab.a_norm(),
        .b = C_clipped * ab.b_norm(),
    });
    return Lrgb {
        .r = details::ct::clamp(res.r, 0., 1.),
        .g = details::ct::clamp(res.g, 0., 1.),
        .b = details::ct::clamp(res.b, 0., 1.),
    };
}

constexpr auto is_representable(Oklab lab) -> bool {
    /*
    constexpr auto eps = 0.;  // 1e-3;
    constexpr auto low = 0. - eps;
    constexpr auto high = 1. + eps;

    const auto lrgb = to_lrgb(lab);
    return lrgb.r >= low && lrgb.g >= low && lrgb.b >= low &&  //
           lrgb.r <= high && lrgb.g <= high && lrgb.b <= high;
   */

    constexpr auto low = -0.49;
    constexpr auto high = 255.49;

    const auto rgb = to_rgb(to_lrgb(lab));

    return rgb.r >= low && rgb.g >= low && rgb.b >= low &&  //
           rgb.r <= high && rgb.g <= high && rgb.b <= high;
}

}  // namespace details::ct

namespace defaults {

constexpr static auto dark_mode_gray = uint8_t {0x12};

constexpr static auto dark_mode_rgb = Rgb {
    .r = dark_mode_gray,
    .g = dark_mode_gray,
    .b = dark_mode_gray,
};
constexpr static auto dark_mode_oklab = to_oklab(to_lrgb(dark_mode_rgb));

}  // namespace defaults

namespace details::ct {

constexpr auto get_radius_down(Oklab lab) -> double {
    constexpr auto eps = 1e-5;

    if (lab.l > 1. - eps) {
        return 0.;
    };

    return details::ct::hypot(lab.l - 1., lab.a, lab.b);
}

constexpr auto get_angle_down(Oklab lab) -> double {
    constexpr auto eps = 1e-5;

    if (lab.l < eps || lab.l > (1. - eps)) {
        return 0.;
    };

    const auto c = details::ct::hypot(lab.a, lab.b);
    return details::ct::atan(c / (1. - lab.l));
}

constexpr auto from_angle_down(double l_radius, ab_norm_t ab, double beta) -> Oklab {
    const auto c = l_radius * details::ct::sin(beta);
    Expects(c >= 0);

    return Oklab {
        .l = 1. - l_radius * details::ct::cos(beta),
        .a = c * ab.a_norm(),
        .b = c * ab.b_norm(),
    };
};

constexpr auto get_radius_up(Oklab lab) -> double {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    constexpr auto eps = 1e-5;

    if (lab.l < l_dark + eps) {
        return 0.;
    };

    return details::ct::hypot(lab.l - l_dark, lab.a, lab.b);
}

constexpr auto get_angle_up(Oklab lab) -> double {
    constexpr auto eps = 1e-5;
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    if (lab.l < l_dark + eps || lab.l > (1. - eps)) {
        return 0.;
    };

    const auto c = details::ct::hypot(lab.a, lab.b);
    return details::ct::atan(c / (lab.l - l_dark));
}

constexpr auto from_angle_up(double l_radius, ab_norm_t ab, double beta) -> Oklab {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    const auto c = l_radius * details::ct::sin(beta);
    Expects(c >= 0);

    return Oklab {
        .l = l_dark + l_radius * details::ct::cos(beta),
        .a = c * ab.a_norm(),
        .b = c * ab.b_norm(),
    };
}

constexpr auto max_circle_angle_down(double l_radius, ab_norm_t ab) -> double {
    constexpr auto eps = 1e-5;

    if (l_radius < eps || l_radius > (1. - eps)) {
        return 0.;
    };

    /*
    const auto cusp = find_cusp(a0, b0);
    Expects(cusp.L > eps);
    Expects(cusp.C > eps);
    const auto alpha_rad = details::ct::atanf(cusp.C / (1. - cusp.L));
    */
    const auto alpha_rad = std::numbers::pi / 2.;

    // Note, pass a__ and b__ as parameters, not captures, to make lambda constexpr
    const auto is_rep = [&](ab_norm_t ab_, double beta_) constexpr -> bool {
        return is_representable(from_angle_down(l_radius, ab_, beta_));
    };

    /*
    if (is_rep(a0, b0, alpha_rad)) {
        return alpha_rad;
    }
    */

    auto low = 0.;
    auto high = alpha_rad;
    // auto low = 0. + 0.5;
    // auto high = alpha_rad * 0. + 0.52;

    for (auto i = 0; i < 20; ++i) {
        const auto mid = (low + high) / 2.;

        if (is_rep(ab, mid)) {
            low = mid;
        } else {
            high = mid;
        }
    }

    return low;
}

constexpr auto max_circle_angle_up(double l_radius, ab_norm_t ab) -> double {
    constexpr auto deg_to_rad = std::numbers::pi / 180.;
    constexpr auto eps = 1e-5;
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    if (l_radius < eps || l_radius + l_dark > (1. - eps)) {
        return 0.;
    };

    const auto cusp = find_cusp(ab);
    Expects(cusp.L > eps);
    Expects(cusp.C > eps);
    const auto alpha_cups = details::ct::atan(cusp.C / std::max(eps, cusp.L - l_dark));

    // pass a__ and b__ as parameters, not captures, to make lambda constexpr
    auto is_rep = [&](ab_norm_t ab_, double beta_) constexpr -> bool {
        return is_representable(from_angle_up(l_radius, ab_, beta_));
    };

    // Using slope along alpha_cups would cut a lot of colors below the cups point,
    // To make the search curve cut the real gamut curve earlier an additional angle
    // must be added.
    constexpr auto slope_angle_adjust = 10. * deg_to_rad;
    // Angles too close to 90 degrees create ambiguity for colors close to dark point.
    // To make the colors invertable, the slope needs to be limited to a value
    // smaller then 90 degrees.
    constexpr auto slope_angle_max = 80. * deg_to_rad;
    const auto alpha_rad = std::min(alpha_cups + slope_angle_adjust, slope_angle_max);

    if (is_rep(ab, alpha_rad)) {
        return alpha_rad;
    }

    auto low = 0.;
    auto high = alpha_rad;

    for (auto i = 0; i < 20; ++i) {
        const auto mid = (low + high) / 2.;

        if (is_rep(ab, mid)) {
            low = mid;
        } else {
            high = mid;
        }
    }

    return low;
}

}  // namespace details::ct

constexpr auto to_dark_mode(Rgb rgb) -> Rgb {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    const auto lab = to_oklab(to_lrgb(rgb));
    const auto lch = to_oklch(lab);
    const auto ab = details::ct::ab_norm_t {lab};

    // distance and angles
    const auto r_light = details::ct::get_radius_down(lab);
    const auto r_dark = r_light * (1. - l_dark);

    const auto b_light_max = details::ct::max_circle_angle_down(r_light, ab);
    const auto b_dark_max = details::ct::max_circle_angle_up(r_dark, ab);

    const auto b_light = details::ct::get_angle_down(lab);
    const auto b_light_ratio =
        details::ct::clamp(b_light / std::max(1e-5, b_light_max), 0., 1.);

    // derive
    const auto b_dark_ratio = b_light_ratio;
    const auto b_dark = b_dark_max * b_dark_ratio;
    const auto lch1 = Oklch {
        .l = l_dark + r_dark * details::ct::cos(b_dark),
        .c = r_dark * details::ct::sin(b_dark),
        .h = lch.h,
    };
    return to_rgb(to_lrgb(to_oklab(lch1)));
}

constexpr auto to_light_mode(Rgb rgb) -> Rgb {
    constexpr auto l_dark = defaults::dark_mode_oklab.l;

    const auto lab = to_oklab(to_lrgb(rgb));
    const auto lch = to_oklch(lab);
    const auto ab = details::ct::ab_norm_t {lab};

    // distance and angles
    const auto r_dark = details::ct::get_radius_up(lab);
    const auto r_light = r_dark / (1. - l_dark);

    const auto b_dark_max = details::ct::max_circle_angle_up(r_dark, ab);
    const auto b_light_max = details::ct::max_circle_angle_down(r_light, ab);

    const auto b_dark = details::ct::get_angle_up(lab);
    const auto b_dark_ratio =
        details::ct::clamp(b_dark / std::max(1e-5, b_dark_max), 0., 1.);

    // derive
    const auto b_light_ratio = b_dark_ratio;
    const auto b_light = b_light_max * b_light_ratio;
    const auto lch1 = Oklch {
        .l = 1. - r_light * details::ct::cos(b_light),
        .c = r_light * details::ct::sin(b_light),
        .h = lch.h,
    };
    return to_rgb(to_lrgb(to_oklab(lch1)));
}

//
// Static testing
//

namespace details::ct {

constexpr static inline auto test_rgb = Rgb {
    .r = 0,
    .g = 255,
    .b = 128,
};
constexpr static inline auto test_lrgb = Lrgb {
    .r = 0,
    .g = 1.,
    .b = 0.21586050011389926,
};
constexpr static inline auto test_oklab = Oklab {
    .l = 0.8750742367323052,
    .a = -0.2054117467195273,
    .b = 0.11299691103557084,
};
constexpr static inline auto test_oklch = Oklch {
    .l = 0.8750742367323052,
    .c = 0.23444037108388127,
    .h = 151.1848269852735,
};

static_assert(is_close(to_lrgb(test_rgb), test_lrgb));
static_assert(is_close(to_rgb(test_lrgb), test_rgb));

static_assert(is_close(to_oklab(test_lrgb), test_oklab));
static_assert(is_close(to_lrgb(test_oklab), test_lrgb));

static_assert(is_close(to_oklch(test_oklab), test_oklch));
static_assert(is_close(to_oklab(test_oklch), test_oklab));

}  // namespace details::ct

}  // namespace logicsim

#endif
