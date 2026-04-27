
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
#include <iostream>

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

template <bool dark_mode>
auto image_to_darklight(BLImageData data) -> void {
    Expects(data.format == BL_FORMAT_PRGB32);
    Expects(data.flags == BL_FORMAT_NO_FLAGS);
    Expects(data.pixel_data != nullptr);

    auto t = Timer {};

    auto pixel_data = reinterpret_cast<BLRgba32*>(data.pixel_data);
    const auto stride = gsl::narrow<std::size_t>(data.stride) / sizeof(BLRgba32);

    for (auto y : range(gsl::narrow<std::size_t>(data.size.h))) {
        for (auto x : range(gsl::narrow<std::size_t>(data.size.w))) {
            auto& pixel = pixel_data[x + y * stride];

            const auto rgb = Rgb {
                .r = static_cast<float>(pixel.r()),
                .g = static_cast<float>(pixel.g()),
                .b = static_cast<float>(pixel.b()),
            };
            const auto res = dark_mode ? to_dark_mode(rgb) : to_light_mode(rgb);

            pixel.setR(static_cast<uint32_t>(std::clamp(std::round(res.r), 0.f, 255.f)));
            pixel.setG(static_cast<uint32_t>(std::clamp(std::round(res.g), 0.f, 255.f)));
            pixel.setB(static_cast<uint32_t>(std::clamp(std::round(res.b), 0.f, 255.f)));
        }
    }
    print(t.delta_ms() / (data.size.h * data.size.w) * 1000., "us");
}

[[nodiscard]] auto get_diff(uint32_t a, uint32_t b) -> uint32_t {
    return std::clamp(128 + 1 * (a - b), uint32_t {0}, uint32_t {255});
}

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

    for (auto y : range(gsl::narrow<std::size_t>(data.size.h))) {
        for (auto x : range(gsl::narrow<std::size_t>(data.size.w))) {
            auto& p_diff = pd_diff[x + y * stride];
            auto& p_orig = pd_orig[x + y * stride];

            p_diff.setR(get_diff(p_diff.r(), p_orig.r()));
            p_diff.setG(get_diff(p_diff.g(), p_orig.g()));
            p_diff.setB(get_diff(p_diff.b(), p_orig.b()));
        }
    }

    print(t.delta_ms() / (diff.size.h * diff.size.w) * 1000., "us");
}

auto image_transform() -> void {
    auto img = BLImage {};
    check(img.read_from_file("light_input.png"));
    check(img.convert(BL_FORMAT_PRGB32));
    print(img.width());

    auto data = BLImageData {};
    img.make_mutable(&data);

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
