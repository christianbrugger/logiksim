#ifndef LOGICSIM_CORE_RENDER_COLOR_TRANSFORM_H
#define LOGICSIM_CORE_RENDER_COLOR_TRANSFORM_H

#include <gcem.hpp>

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
    float r;
    float g;
    float b;

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
    float r;
    float g;
    float b;

    [[nodiscard]] constexpr auto operator==(const Lrgb&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] constexpr auto is_close(const Lrgb& x, const Lrgb& y) -> bool;

/*
 * @brief: Oklab color space
 */
struct Oklab {
    float l;
    float a;
    float b;

    [[nodiscard]] constexpr auto operator==(const Oklab&) const -> bool = default;
    [[nodiscard]] auto format() const -> std::string;
};

[[nodiscard]] constexpr auto is_close(const Oklab& x, const Oklab& y) -> bool;

[[nodiscard]] constexpr auto to_lrgb(Rgb c) -> Lrgb;
[[nodiscard]] constexpr auto to_rgb(Lrgb c) -> Rgb;
[[nodiscard]] constexpr auto to_oklab(Lrgb c) -> Oklab;
[[nodiscard]] constexpr auto to_lrgb(Oklab c) -> Lrgb;

//
// Implementation
//

namespace details::ct {

[[nodiscard]] constexpr auto abs(float x) -> float {
    if (std::is_constant_evaluated()) {
        return gcem::abs(x);
    }
    return std::abs(x);
}

[[nodiscard]] constexpr auto powf(float x, float y) -> float {
    if (std::is_constant_evaluated()) {
        return gcem::pow(x, y);
    }
    return std::powf(x, y);
}

[[nodiscard]] constexpr auto cbrtf(float x) -> float {
    if (std::is_constant_evaluated()) {
        if (x == 0.0f) {
            return 0.0f;
        }
        // use double to get full float precision
        if (x < 0.0f) {
            return static_cast<float>(-gcem::pow(-double {x}, 1.0 / 3.0));
        }
        return static_cast<float>(gcem::pow(double {x}, 1.0 / 3.0));
    }
    return std::cbrtf(x);
}

[[nodiscard]] constexpr auto sqrtf(float x) -> float {
    if (std::is_constant_evaluated()) {
        return gcem::sqrt(x);
    }
    return std::sqrtf(x);
}

[[nodiscard]] constexpr auto clamp(float x, float min, float max) -> float {
    if (x < min) {
        return min;
    }
    if (x > max) {
        return max;
    }

    return x;
}

[[nodiscard]] constexpr auto sgn(float x) -> float {
    return static_cast<float>(0.f < x) - static_cast<float>(x < 0.f);
}

}  // namespace details::ct

constexpr auto is_close(const Rgb& x, const Rgb& y) -> bool {
    constexpr auto tol = 255.0f * 1e-6f;
    return details::ct::abs(x.r - y.r) < tol &&  //
           details::ct::abs(x.g - y.g) < tol &&  //
           details::ct::abs(x.b - y.b) < tol;
}

constexpr auto is_close(const Lrgb& x, const Lrgb& y) -> bool {
    constexpr auto tol = 1e-6f;
    return details::ct::abs(x.r - y.r) < tol &&  //
           details::ct::abs(x.g - y.g) < tol &&  //
           details::ct::abs(x.b - y.b) < tol;
}

constexpr auto is_close(const Oklab& x, const Oklab& y) -> bool {
    constexpr auto tol = 1e-6f;
    return details::ct::abs(x.l - y.l) < tol &&  //
           details::ct::abs(x.a - y.a) < tol &&  //
           details::ct::abs(x.b - y.b) < tol;
}

namespace details::ct {

constexpr auto to_lrgb(float value) -> float {
    if (value > 0.0f) {
        value /= 255.0f;
        if (value <= 0.04045f) {
            return value / 12.92f;
        }
        return details::ct::powf((value + 0.055f) / 1.055f, 2.4f);
    }

    value = -value;
    value /= 255.0f;
    if (value <= 0.04045f) {
        return -(value / 12.92f);
    }
    return -(details::ct::powf((value + 0.055f) / 1.055f, 2.4f));
}

constexpr auto to_rgb(float value) -> float {
    if (value >= 0.0f) {
        if (value <= 0.0031308f) {
            return 12.92f * value * 255.0f;
        }
        return (1.055f * details::ct::powf(value, 1.0f / 2.4f) - 0.055f) * 255.0f;
    }

    value = -value;
    if (value <= 0.0031308f) {
        return -(12.92f * value * 255.0f);
    }
    return -((1.055f * details::ct::powf(value, 1.0f / 2.4f) - 0.055f) * 255.0f);
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

constexpr auto to_oklab(Lrgb c) -> Oklab {
    const auto l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
    const auto m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
    const auto s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

    const auto l_ = details::ct::cbrtf(l);
    const auto m_ = details::ct::cbrtf(m);
    const auto s_ = details::ct::cbrtf(s);

    return Oklab {
        .l = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
        .a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
        .b = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_,
    };
}

constexpr auto to_lrgb(Oklab c) -> Lrgb {
    const auto l_ = c.l + 0.3963377774f * c.a + 0.2158037573f * c.b;
    const auto m_ = c.l - 0.1055613458f * c.a - 0.0638541728f * c.b;
    const auto s_ = c.l - 0.0894841775f * c.a - 1.2914855480f * c.b;

    const auto l = l_ * l_ * l_;
    const auto m = m_ * m_ * m_;
    const auto s = s_ * s_ * s_;

    return Lrgb {
        .r = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
        .g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
        .b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
    };
}

namespace details::ct {

/*
 * Finds the maximum saturation possible for a given hue that fits in sRGB
 *
 * Saturation here is defined as S = C/L
 * a and b must be normalized so a^2 + b^2 == 1
 *
 * Max saturation will be when one of r, g or b goes below zero.
 */
[[nodiscard]] constexpr auto compute_max_saturation(float a, float b) -> float {
    // Select different coefficients depending on which component goes below zero first
    float k0 = {};
    float k1 = {};
    float k2 = {};
    float k3 = {};
    float k4 = {};
    float wl = {};
    float wm = {};
    float ws = {};

    if (-1.88170328f * a - 0.80936493f * b > 1) {
        // Red component
        k0 = +1.19086277f;
        k1 = +1.76576728f;
        k2 = +0.59662641f;
        k3 = +0.75515197f;
        k4 = +0.56771245f;
        wl = +4.0767416621f;
        wm = -3.3077115913f;
        ws = +0.2309699292f;
    } else if (1.81444104f * a - 1.19445276f * b > 1) {
        // Green component
        k0 = +0.73956515f;
        k1 = -0.45954404f;
        k2 = +0.08285427f;
        k3 = +0.12541070f;
        k4 = +0.14503204f;
        wl = -1.2684380046f;
        wm = +2.6097574011f;
        ws = -0.3413193965f;
    } else {
        // Blue component
        k0 = +1.35733652f;
        k1 = -0.00915799f;
        k2 = -1.15130210f;
        k3 = -0.50559606f;
        k4 = +0.00692167f;
        wl = -0.0041960863f;
        wm = -0.7034186147f;
        ws = +1.7076147010f;
    }

    // Approximate max saturation using a polynomial:
    const auto S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

    // Do one step Halley's method to get closer
    // this gives an error less than 10e6, except for some blue hues where the dS/dh is
    // close to infinite this should be sufficient for most applications, otherwise do
    // two/three steps

    const auto k_l = +0.3963377774f * a + 0.2158037573f * b;
    const auto k_m = -0.1055613458f * a - 0.0638541728f * b;
    const auto k_s = -0.0894841775f * a - 1.2914855480f * b;

    {
        const auto l_ = 1.f + S * k_l;
        const auto m_ = 1.f + S * k_m;
        const auto s_ = 1.f + S * k_s;

        const auto l = l_ * l_ * l_;
        const auto m = m_ * m_ * m_;
        const auto s = s_ * s_ * s_;

        const auto l_dS = 3.f * k_l * l_ * l_;
        const auto m_dS = 3.f * k_m * m_ * m_;
        const auto s_dS = 3.f * k_s * s_ * s_;

        const auto l_dS2 = 6.f * k_l * k_l * l_;
        const auto m_dS2 = 6.f * k_m * k_m * m_;
        const auto s_dS2 = 6.f * k_s * k_s * s_;

        const auto f = wl * l + wm * m + ws * s;
        const auto f1 = wl * l_dS + wm * m_dS + ws * s_dS;
        const auto f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

        return S - f * f1 / (f1 * f1 - 0.5f * f * f2);
    }
}

/*
 * @brief: finds L_cusp and C_cusp for a given hue
 *
 * a and b must be normalized so a^2 + b^2 == 1
 */
struct LC {
    float L;
    float C;
};

[[nodiscard]] constexpr auto find_cusp(float a, float b) -> LC {
    // First, find the maximum saturation (saturation S = C/L)
    const auto S_cusp = compute_max_saturation(a, b);

    // Convert to linear sRGB to find the first point where at least one of r,g or b >= 1:
    const Lrgb rgb_at_max = to_lrgb(Oklab {
        .l = 1,
        .a = S_cusp * a,
        .b = S_cusp * b,
    });
    const auto L_cusp =
        details::ct::cbrtf(1.f / std::max({rgb_at_max.r, rgb_at_max.g, rgb_at_max.b}));
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
[[nodiscard]] constexpr auto find_gamut_intersection(float a, float b, float L1, float C1,
                                                     float L0) -> float {
    // Find the cusp of the gamut triangle
    const auto cusp = find_cusp(a, b);

    // Find the intersection for upper and lower half seprately
    float t = {};
    if (((L1 - L0) * cusp.C - (cusp.L - L0) * C1) <= 0.f) {
        // Lower half

        t = cusp.C * L0 / (C1 * cusp.L + cusp.C * (L0 - L1));
    } else {
        // Upper half

        // First intersect with triangle
        t = cusp.C * (L0 - 1.f) / (C1 * (cusp.L - 1.f) + cusp.C * (L0 - L1));

        // Then one step Halley's method
        {
            float dL = L1 - L0;
            float dC = C1;

            float k_l = +0.3963377774f * a + 0.2158037573f * b;
            float k_m = -0.1055613458f * a - 0.0638541728f * b;
            float k_s = -0.0894841775f * a - 1.2914855480f * b;

            float l_dt = dL + dC * k_l;
            float m_dt = dL + dC * k_m;
            float s_dt = dL + dC * k_s;

            // If higher accuracy is required, 2 or 3 iterations of the following block
            // can be used:
            {
                float L = L0 * (1.f - t) + t * L1;
                float C = t * C1;

                float l_ = L + C * k_l;
                float m_ = L + C * k_m;
                float s_ = L + C * k_s;

                float l = l_ * l_ * l_;
                float m = m_ * m_ * m_;
                float s = s_ * s_ * s_;

                float ldt = 3 * l_dt * l_ * l_;
                float mdt = 3 * m_dt * m_ * m_;
                float sdt = 3 * s_dt * s_ * s_;

                float ldt2 = 6 * l_dt * l_dt * l_;
                float mdt2 = 6 * m_dt * m_dt * m_;
                float sdt2 = 6 * s_dt * s_dt * s_;

                float r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s - 1;
                float r1 =
                    4.0767416621f * ldt - 3.3077115913f * mdt + 0.2309699292f * sdt;
                float r2 =
                    4.0767416621f * ldt2 - 3.3077115913f * mdt2 + 0.2309699292f * sdt2;

                float u_r = r1 / (r1 * r1 - 0.5f * r * r2);
                float t_r = -r * u_r;

                float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s - 1;
                float g1 =
                    -1.2684380046f * ldt + 2.6097574011f * mdt - 0.3413193965f * sdt;
                float g2 =
                    -1.2684380046f * ldt2 + 2.6097574011f * mdt2 - 0.3413193965f * sdt2;

                float u_g = g1 / (g1 * g1 - 0.5f * g * g2);
                float t_g = -g * u_g;

                float b_ = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s - 1;
                float b1 =
                    -0.0041960863f * ldt - 0.7034186147f * mdt + 1.7076147010f * sdt;
                float b2 =
                    -0.0041960863f * ldt2 - 0.7034186147f * mdt2 + 1.7076147010f * sdt2;

                float u_b = b1 / (b1 * b1 - 0.5f * b_ * b2);
                float t_b = -b_ * u_b;

                t_r = u_r >= 0.f ? t_r : FLT_MAX;
                t_g = u_g >= 0.f ? t_g : FLT_MAX;
                t_b = u_b >= 0.f ? t_b : FLT_MAX;

                t += std::min({t_r, t_g, t_b});
            }
        }
    }

    return t;
}

[[nodiscard]] constexpr auto gamut_clip_preserve_chroma(Lrgb rgb) -> Lrgb {
    if (rgb.r <= 1.0f && rgb.g <= 1.0f && rgb.b <= 1.0f &&  //
        rgb.r >= 0.0f && rgb.g >= 0.0f && rgb.b >= 0.0f) {
        return rgb;
    }

    const auto lab = to_oklab(rgb);

    const auto L = lab.l;
    const auto eps = 0.00001f;
    const auto C = std::max(eps, details::ct::sqrtf(lab.a * lab.a + lab.b * lab.b));
    const auto a_ = lab.a / C;
    const auto b_ = lab.b / C;

    const auto L0 = details::ct::clamp(L, 0.0f, 1.0f);

    const auto t = find_gamut_intersection(a_, b_, L, C, L0);
    const auto L_clipped = L0 * (1.0f - t) + t * L;
    const auto C_clipped = t * C;

    const auto res = to_lrgb(Oklab {
        .l = L_clipped,
        .a = C_clipped * a_,
        .b = C_clipped * b_,
    });
    return Lrgb {
        .r = details::ct::clamp(res.r, 0.0f, 1.0f),
        .g = details::ct::clamp(res.g, 0.0f, 1.0f),
        .b = details::ct::clamp(res.b, 0.0f, 1.0f),
    };
}

}  // namespace details::ct

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
    .g = 1.0,
    .b = 0.215860500f,
};
constexpr static inline auto test_oklab = Oklab {
    .l = 0.875074241f,
    .a = -0.205411749f,
    .b = 0.112996876f,
};

static_assert(is_close(to_lrgb(test_rgb), test_lrgb));
static_assert(is_close(to_rgb(test_lrgb), test_rgb));
static_assert(is_close(to_oklab(test_lrgb), test_oklab));
static_assert(is_close(to_lrgb(test_oklab), test_lrgb));

// constexpr static auto abc = details::ct::gamut_clip_preserve_chroma(Lrgb(1.5, 0.5,
// 0.5));

}  // namespace details::ct

}  // namespace logicsim

#endif
