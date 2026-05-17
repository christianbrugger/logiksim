
#include "core/render/color_transform.h"

#include "core/logging.h"
#include "core/timer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

//
// Constexpr testing
//

namespace details::ct {

constexpr static inline auto test_rgbi = RgbI {
    .r = 0,
    .g = 255,
    .b = 128,
};
constexpr static inline auto test_rgb = Rgb {
    .r = 0.,
    .g = 255.,
    .b = 128.,
};
constexpr static inline auto test_lrgb = Lrgb {
    .r = 0.,
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

static_assert(to_rgb(test_rgbi) == test_rgb);
static_assert(to_rgbi(test_rgb) == test_rgbi);

static_assert(is_close(to_lrgb(test_rgb), test_lrgb));
static_assert(is_close(to_rgb(test_lrgb), test_rgb));

static_assert(is_close(to_oklab(test_lrgb), test_oklab));
static_assert(is_close(to_lrgb(test_oklab), test_lrgb));

static_assert(is_close(to_oklch(test_oklab), test_oklch));
static_assert(is_close(to_oklab(test_oklch), test_oklab));

// static_assert(to_dark_mode(test_rgbi).r >= 0);

}  // namespace details::ct

//
// Runtime tests
//

TEST(RenderColorTransform, oklab) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));

    EXPECT_DOUBLE_EQ(lab.l, 0.44528036461367088);
    EXPECT_DOUBLE_EQ(lab.a, -0.031973493628807574);
    EXPECT_DOUBLE_EQ(lab.b, -0.30688751589479746);
}

TEST(RenderColorTransform, oklch) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lch = to_oklch(to_oklab(to_lrgb(rgb)));

    EXPECT_DOUBLE_EQ(lch.l, 0.4452803646136708);
    EXPECT_DOUBLE_EQ(lch.c, 0.3085486213012642);
    EXPECT_DOUBLE_EQ(lch.h, 264.05202063805501);
}

TEST(RenderColorTransform, RoundtripLrgb) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lrgb = to_lrgb(rgb);
    const auto res = to_rgb(lrgb);

    EXPECT_DOUBLE_EQ(res.r, rgb.r);
    EXPECT_DOUBLE_EQ(res.g, rgb.g);
    EXPECT_DOUBLE_EQ(res.b, rgb.b);
}

TEST(RenderColorTransform, RoundtripOklab) {
    constexpr auto eps = 1e-15;

    const auto lrgb = Lrgb {.r = 0, .g = 0, .b = 0.956};
    const auto oklab = to_oklab(lrgb);
    const auto res = to_lrgb(oklab);

    EXPECT_LT(std::abs(res.r - lrgb.r), eps);
    EXPECT_LT(std::abs(res.g - lrgb.g), eps);
    EXPECT_LT(std::abs(res.b - lrgb.b), eps);
}

TEST(RenderColorTransform, RoundtripOklch) {
    constexpr auto eps = 1e-15;

    const auto oklab = Oklab {
        .l = 0.4452845018156794,
        .a = -0.031973790701870744,
        .b = -0.3068903672571033,
    };
    const auto oklch = to_oklch(oklab);
    const auto res = to_oklab(oklch);

    EXPECT_LT(std::abs(res.l - oklab.l), eps);
    EXPECT_LT(std::abs(res.a - oklab.a), eps);
    EXPECT_LT(std::abs(res.b - oklab.b), eps);
}

TEST(RenderColorTransform, RoundtripFull) {
    constexpr auto eps = 255 * 1e-14;

    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lch = to_oklch(to_oklab(to_lrgb(rgb)));
    const auto res = to_rgb(to_lrgb(to_oklab(lch)));

    EXPECT_LT(std::abs(res.r - rgb.r), eps);
    EXPECT_LT(std::abs(res.g - rgb.g), eps);
    EXPECT_LT(std::abs(res.b - rgb.b), eps);
}

TEST(RenderColorTransform, abnorm1) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));

    const auto ab = details::ct::ab_norm_t {lab};

    EXPECT_DOUBLE_EQ(ab.c(), 0.3085486213012642);
    EXPECT_DOUBLE_EQ(ab.a_norm(), -0.10362546263847645);
    EXPECT_DOUBLE_EQ(ab.b_norm(), -0.99461639011880443);
}

TEST(RenderColorTransform, abnorm2) {
    const auto lab = Oklab {.l = 0.4, .a = 0.5, .b = 0.2};

    const auto ab = details::ct::ab_norm_t {lab};

    EXPECT_DOUBLE_EQ(ab.c(), 0.5385164807134505);
    EXPECT_DOUBLE_EQ(ab.a_norm(), 0.9284766908852592);
    EXPECT_DOUBLE_EQ(ab.b_norm(), 0.37139067635410367);
}

TEST(RenderColorTransform, MaxChromaBench) {
    {
        auto t = Timer {};

        const auto num = 100;
        auto total = 0.;
        for (auto i = 0; i < num; ++i) {
            const auto rgb = Rgb {.r = 0, .g = 0, .b = 2. * i};
            const auto lab = to_oklab(to_lrgb(rgb));

            const auto r = details::ct::get_radius_down(lab);
            const auto ab = details::ct::ab_norm_t(lab);
            total += details::ct::max_circle_angle_down_slow(r, ab);
        }

        Expects(total > 10.);
        print("Time angle_down:", t.delta_seconds() / num * 1000 * 1000, "us");
    }
}

TEST(RenderColorTransform, MaxAngleDownHard1) {
    // This RGB values is very tricky, as it is convex

    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));

    const auto r = details::ct::get_radius_down(lab);
    const auto ab = details::ct::ab_norm_t(lab);
    const auto angle_found = details::ct::max_circle_angle_down_slow(r, ab);

    const auto angle_expected = details::ct::get_angle_down(lab);

    EXPECT_LT(std::abs(angle_found - angle_expected), 1e-12);
}

TEST(RenderColorTransform, MaxAngleDownHard2) {
    // TODO:
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 251};
    const auto lab = to_oklab(to_lrgb(rgb));

    const auto r = details::ct::get_radius_down(lab);
    const auto ab = details::ct::ab_norm_t(lab);
    const auto angle_found = details::ct::max_circle_angle_down_slow(r, ab);

    const auto angle_expected = details::ct::get_angle_down(lab);

    EXPECT_LT(std::abs(angle_found - angle_expected), 1e-12);
}

TEST(RenderColorTransform, MaxAngleDownSlow) {
    // This RGB values is very tricky, as it is convex

    // Rgb(41, 95, 236)

    const auto rgb = Rgb {.r = 41, .g = 95, .b = 236};
    const auto lab = to_oklab(to_lrgb(rgb));
    const auto ab = details::ct::ab_norm_t(lab);

    const auto r_dark = 0.41671095956483784;
    const auto b_found = details::ct::max_circle_angle_up_slow(r_dark, ab);
    print(b_found);

    /*
    const auto r = details::ct::get_radius_down(lab);
    const auto angle_found = details::ct::max_circle_angle_down_slow(r, ab);

    const auto angle_expected = details::ct::get_angle_down(lab);
    print(angle_found);
    print(angle_expected);

    const auto angle_found2 = details::ct::max_circle_angle_down(r, ab);
    print(angle_found2);

    print("==");

    const auto ru = details::ct::get_radius_up(lab);
    const auto b_found = details::ct::max_circle_angle_up_slow(ru, ab);
    const auto b_expected = details::ct::get_angle_up(lab);
    const auto b_found2 = details::ct::max_circle_angle_up(ru, ab);

    print(lab);
    print(b_found);
    print(b_expected);
    print(b_found2);

    print("===");
    print(rgb);
    const auto dark = to_dark_mode(rgb);
    print(dark);
    const auto light = to_light_mode(dark);
    print(light);
    */
}

TEST(RenderColorTransform, toDarkMax) {
    const auto rgb = RgbI {.r = 255, .g = 255, .b = 255};
    const auto dark = to_dark_mode(rgb);

    EXPECT_EQ(dark, defaults::dark_mode_rgbi);
}

TEST(RenderColorTransform, toLightVeryDark) {
    constexpr auto white = RgbI {.r = 255, .g = 255, .b = 255};

    for (auto i = std::uint8_t {0}; i <= defaults::dark_mode_gray; ++i) {
        const auto rgb = RgbI {.r = i, .g = i, .b = i};

        const auto light = to_light_mode(rgb);
        EXPECT_EQ(light, white);
    }
}

/*
TEST(RenderColorTransform, cups) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));
    const auto ab = details::ct::ab_norm_t {lab};

    const auto cups = details::ct::find_cusp(ab);

    const auto l = 0.4957508;
    const auto c = 0.28383392;

    EXPECT_DOUBLE_EQ(cups.L, l);
    EXPECT_DOUBLE_EQ(cups.C, c);
}

TEST(RenderColorTransform, fromup) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));
    const auto ab = details::ct::ab_norm_t {lab};

    const auto l_radius = details::ct::get_radius_down(lab);
    const auto beta = details::ct::get_angle_down(lab);

    EXPECT_DOUBLE_EQ(l_radius, 0.6347566772311115);
    EXPECT_DOUBLE_EQ(beta, 0.5076095);

    const auto res = details::ct::from_angle_down(l_radius, ab, beta);

    EXPECT_TRUE(is_close(lab, res));

    print("0", to_oklch(lab));

    const auto r1 = details::ct::from_angle_down(l_radius, ab, 0.40497094);
    print("r", details::ct::get_radius_down(r1));
    print("b", details::ct::get_angle_down(r1));
    print("1", to_oklch(r1));
    print("1", to_rgb(to_lrgb(r1)));

    const auto cusp0 = details::ct::find_cusp(ab);
    print(cusp0.L, cusp0.C);
    const auto cusp1 = details::ct::find_cusp(details::ct::ab_norm_t {r1});
    print(cusp1.L, cusp1.C);
}
*/

}  // namespace logicsim
