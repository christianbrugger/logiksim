
#include "core/render/color_transform.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(RenderColorTransform, oklab) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));

    const auto l = 0.44528043;
    const auto a = -0.031973623;
    const auto b = -0.30688748;

    EXPECT_FLOAT_EQ(lab.l, l);
    EXPECT_FLOAT_EQ(lab.a, a);
    EXPECT_FLOAT_EQ(lab.b, b);
}

TEST(RenderColorTransform, oklch) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lch = to_oklch(to_oklab(to_lrgb(rgb)));

    const auto l = 0.44528043;
    const auto c = 0.3085486;
    const auto h = 264.052;

    EXPECT_FLOAT_EQ(lch.l, l);
    EXPECT_FLOAT_EQ(lch.c, c);
    EXPECT_FLOAT_EQ(lch.h, h);
}

TEST(RenderColorTransform, roundtrip) {
    constexpr auto eps = 255 * 1e-5;

    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lch = to_oklch(to_oklab(to_lrgb(rgb)));
    const auto res = to_rgb(to_lrgb(to_oklab(lch)));

    EXPECT_LT(std::abs(res.r - 0), eps);
    EXPECT_LT(std::abs(res.g - 0), eps);
    EXPECT_LT(std::abs(res.b - 250), eps);
}

TEST(RenderColorTransform, abnorm1) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));

    const auto c = 0.3085486;
    const auto a = -0.10362589;
    const auto b = -0.99461633;

    const auto ab = details::ct::ab_norm_t {lab};

    EXPECT_FLOAT_EQ(ab.c(), c);
    EXPECT_FLOAT_EQ(ab.a_norm(), a);
    EXPECT_FLOAT_EQ(ab.b_norm(), b);
}

TEST(RenderColorTransform, abnorm2) {
    const auto lab = Oklab {.l = 0.4f, .a = 0.5f, .b = 0.2f};

    const auto c = 0.5385164807134505;
    const auto a = 0.9284766908852592;
    const auto b = 0.37139067635410367;

    const auto ab = details::ct::ab_norm_t {lab};

    EXPECT_FLOAT_EQ(ab.c(), c);
    EXPECT_FLOAT_EQ(ab.a_norm(), a);
    EXPECT_FLOAT_EQ(ab.b_norm(), b);
}

TEST(RenderColorTransform, cups) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));
    const auto ab = details::ct::ab_norm_t {lab};

    const auto cups = details::ct::find_cusp(ab);

    const auto l = 0.4957508;
    const auto c = 0.28383392;

    EXPECT_FLOAT_EQ(cups.L, l);
    EXPECT_FLOAT_EQ(cups.C, c);
}

TEST(RenderColorTransform, fromup) {
    const auto rgb = Rgb {.r = 0, .g = 0, .b = 250};
    const auto lab = to_oklab(to_lrgb(rgb));
    const auto ab = details::ct::ab_norm_t {lab};

    const auto l_radius = details::ct::get_radius_down(lab);
    const auto beta = details::ct::get_angle_down(lab);

    EXPECT_FLOAT_EQ(l_radius, 0.6347566772311115);
    EXPECT_FLOAT_EQ(beta, 0.5076095);

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

}  // namespace logicsim
