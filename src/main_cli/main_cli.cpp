
#include "core/algorithm/range.h"
#include "core/benchmark/render_line_scene.h"
#include "core/circuit_example.h"
#include "core/default_element_definition.h"
#include "core/editable_circuit.h"
#include "core/format/blend2d_type.h"
#include "core/geometry/scene.h"
#include "core/logging.h"
#include "core/macro/try_catch.h"
#include "core/render/circuit/render_circuit.h"
#include "core/render/color_transform.h"
#include "core/render/image_surface.h"
#include "core/timer.h"
#include "core/vocabulary/widget_render_config.h"

#include <blend2d/blend2d.h>
#include <fmt/format.h>
#include <gsl/gsl>

#include <exception>
#include <execution>
#include <iostream>
#include <ranges>

namespace logicsim {

auto test_render() -> void {
    auto cache = cache_with_default_fonts();

    auto editable_circuit = load_example_with_logging(2);
    visible_selection_select_all(editable_circuit);

    for (auto _ [[maybe_unused]] : range(3)) {
        auto timer = Timer {"Example Circuit Render", Timer::Unit::ms, 3};
        render_layout_to_file(editable_circuit.layout(),
                              editable_circuit.visible_selection(), "test_circuit.png",
                              create_context_render_settings(BLSizeI {800, 600}), cache);
    }
}

auto check(BLResult result) -> void {
    if (result != BL_SUCCESS) {
        throw std::runtime_error(fmt::format("blend2d error result: {}", result));
    }
}

template <bool to_dark>
auto image_to_darklight(BLImageData data) -> void {
    Expects(data.format == BL_FORMAT_PRGB32);
    Expects(data.flags == BL_FORMAT_NO_FLAGS);
    Expects(data.pixel_data != nullptr);

    auto t = Timer {};

    auto pixel_data = reinterpret_cast<BLRgba32*>(data.pixel_data);
    const auto stride = gsl::narrow<std::size_t>(data.stride) / sizeof(BLRgba32);

    const auto width = gsl::narrow<std::size_t>(data.size.w);
    const auto height = gsl::narrow<std::size_t>(data.size.h);

    const auto ys = std::views::iota(std::size_t {0}, height);

    const auto process_row = [pixel_data, width, stride](std::size_t y) {
        for (auto x : range(width)) {
            auto& pixel = pixel_data[x + y * stride];

            const auto rgb = RgbI {
                .r = static_cast<std::uint8_t>(pixel.r()),
                .g = static_cast<std::uint8_t>(pixel.g()),
                .b = static_cast<std::uint8_t>(pixel.b()),
            };
            const auto res = to_dark ? to_dark_mode(rgb) : to_light_mode(rgb);

            pixel.setR(static_cast<uint32_t>(res.r));
            pixel.setG(static_cast<uint32_t>(res.g));
            pixel.setB(static_cast<uint32_t>(res.b));
        }
    };

    std::for_each(std::execution::par_unseq, ys.begin(), ys.end(), process_row);
    print(t.delta_ms() / (data.size.h * data.size.w) * 1000. *
              (std::thread::hardware_concurrency() / 2.),
          "us");
}

[[nodiscard]] auto get_diff(uint32_t a, uint32_t b) -> uint32_t {
    return std::clamp(128 + 1 * (a - b), uint32_t {0}, uint32_t {255});
}

auto get_norm(BLRgba32 a, BLRgba32 b) {
    const auto rgb_a = Rgb {
        .r = static_cast<double>(a.r()),
        .g = static_cast<double>(a.g()),
        .b = static_cast<double>(a.b()),
    };

    const auto rgb_b = Rgb {
        .r = static_cast<double>(b.r()),
        .g = static_cast<double>(b.g()),
        .b = static_cast<double>(b.b()),
    };

    const auto lab_a = to_oklab(to_lrgb(rgb_a));
    const auto lab_b = to_oklab(to_lrgb(rgb_b));

    return std::hypot(lab_a.l - lab_b.l, lab_a.a - lab_b.a, lab_a.b - lab_b.b);

    // return std::hypot(1. * a.r() - b.r(), 1. * a.g() - b.g(), 1. * a.b() - b.b());
};

auto diff_images(BLImageData diff, BLImageData orig) {
    Expects(diff.format == BL_FORMAT_PRGB32);
    Expects(diff.flags == BL_FORMAT_NO_FLAGS);
    Expects(diff.pixel_data != nullptr);

    Expects(orig.format == BL_FORMAT_PRGB32);
    Expects(orig.flags == BL_FORMAT_NO_FLAGS);
    Expects(orig.pixel_data != nullptr);

    Expects(diff.stride == orig.stride);
    Expects(diff.size.w == orig.size.w);
    Expects(diff.size.h == orig.size.h);

    auto t = Timer {};

    auto pd_diff = reinterpret_cast<BLRgba32*>(diff.pixel_data);
    auto pd_orig = reinterpret_cast<BLRgba32*>(orig.pixel_data);
    const auto stride = gsl::narrow<std::size_t>(diff.stride) / sizeof(BLRgba32);

    auto n_max = double {};
    auto x_max = std::size_t {};
    auto y_max = std::size_t {};

    for (auto y : range(gsl::narrow<std::size_t>(diff.size.h))) {
        for (auto x : range(gsl::narrow<std::size_t>(diff.size.w))) {
            auto& p_diff = pd_diff[x + y * stride];
            auto& p_orig = pd_orig[x + y * stride];

            const auto n = get_norm(p_diff, p_orig);
            if (n > n_max) {
                n_max = std::max(n_max, n);
                x_max = x;
                y_max = y;
            }

            p_diff.setR(get_diff(p_diff.r(), p_orig.r()));
            p_diff.setG(get_diff(p_diff.g(), p_orig.g()));
            p_diff.setB(get_diff(p_diff.b(), p_orig.b()));
        }
    }
    print("n_max =", n_max, "x", x_max, "y", y_max);

    print(t.delta_ms() / (diff.size.h * diff.size.w) * 1000., "us");
}

auto single_pixel(BLImageData img) -> void {
    // const auto x = int {1948};
    // const auto y = int {3774};

    // const auto x = int {1965};
    // const auto y = int {3758};

    // const auto x = int {2861};
    // const auto y = int {2996};

    // const auto x = int {1948};
    // const auto y = int {3774};

    // const auto x = int {3616};
    // const auto y = int {3698};

    // const auto x = int {3442};
    // const auto y = int {58};

    const auto x = int {2095};
    const auto y = int {3681};

    // const auto x = int {948};
    // const auto y = int {774};

    Expects(img.format == BL_FORMAT_PRGB32);
    Expects(img.flags == BL_FORMAT_NO_FLAGS);
    Expects(img.pixel_data != nullptr);

    Expects(x < img.size.w);
    Expects(y < img.size.h);

    auto t = Timer {};

    auto pd = reinterpret_cast<BLRgba32*>(img.pixel_data);
    const auto stride = gsl::narrow<std::size_t>(img.stride) / sizeof(BLRgba32);

    auto& p = pd[x + y * stride];

    print("x", x, "y", y);
    print(p.r(), p.g(), p.b());

    // const auto rgb = Rgb {
    //     .r = static_cast<float>(p.r()),
    //     .g = static_cast<float>(p.g()),
    //     .b = static_cast<float>(p.b()),
    // };
    const auto rgb = RgbI {
        .r = static_cast<std::uint8_t>(p.r()),
        .g = static_cast<std::uint8_t>(p.g()),
        .b = static_cast<std::uint8_t>(p.b()),
    };
    print("-> rgb  ", rgb);

    const auto dark = to_dark_mode(rgb);
    // const auto dark = Rgb {
    //     .r = std::clamp(std::round(res.r), 0., 255.),
    //     .g = std::clamp(std::round(res.g), 0., 255.),
    //     .b = std::clamp(std::round(res.b), 0., 255.),
    // };
    print("-> dark ", dark);

    const auto light = to_light_mode(dark);
    // const auto light = Rgb {
    //     .r = std::clamp(std::round(res2.r), 0., 255.),
    //     .g = std::clamp(std::round(res2.g), 0., 255.),
    //     .b = std::clamp(std::round(res2.b), 0., 255.),
    // };
    print("-> light", light);

    // print();
    // print("-> dark (no round)", res);
    // print("-> light (no round)", to_light_mode_raw(res));

    // print();
    // 93.46555336047882, 143.6292677993749, 255.00000000000114
    // const auto a = Rgb {.r = 93, .g = 143, .b = 255};
    // print(a, to_light_mode_raw(a));
    // const auto b = Rgb {.r = 93, .g = 144, .b = 255};
    // print(b, to_light_mode_raw(b));
    // const auto c = Rgb {.r = 94, .g = 143, .b = 255};
    // print(c, to_light_mode_raw(c));
    // const auto d = Rgb {.r = 94, .g = 144, .b = 255};
    // print(d, to_light_mode_raw(d));
    // print();
}

auto image_transform() -> void {
    auto img = BLImage {};
    if (img.read_from_file("light_input.png") != BL_SUCCESS) {
        print("Unable to read file.");
        return;
    }
    check(img.convert(BL_FORMAT_PRGB32));
    print(img.width());

    auto data = BLImageData {};
    img.make_mutable(&data);

    // single_pixel(data);
    // return;

    image_to_darklight<true>(data);
    img.write_to_file("output_dark.png");
    image_to_darklight<false>(data);
    img.write_to_file("output_light.png");

    {
        auto orig = BLImage {};
        check(orig.read_from_file("light_input.png"));
        check(orig.convert(BL_FORMAT_PRGB32));

        auto orig_data = BLImageData {};
        orig.make_mutable(&orig_data);
        diff_images(data, orig_data);
        img.write_to_file("output_diff.png");
    }
}

}  // namespace logicsim

auto main() -> int {
    using namespace logicsim;
    print(try_catch_non_empty_status());

    bool do_run = false;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    try {
        logicsim::image_transform();
    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return -1;
    }

    if (do_run) {
        test_render();

        try {
            auto timer = Timer {"Benchmark + Render", Timer::Unit::ms, 3};
            // auto count = benchmark_simulation(6, 10, true);
            // auto count = logicsim::benchmark_simulation(BENCHMARK_DEFAULT_ELEMENTS,
            //                                            BENCHMARK_DEFAULT_EVENTS, true);

            auto count = logicsim::benchmark_line_renderer(100, true);
            print_fmt("count = {}\n", count);

        } catch (const std::exception& exc) {
            std::cerr << exc.what() << '\n';
            return -1;
        }
    }

    return 0;
}
