
#include "editable_circuit/cache/connection_cache.h"
#include "editable_circuit/cache/helper.h"
#include "editable_circuit/editable_circuit.h"
#include "layout_calculation.h"
#include "render_benchmark.h"
#include "render_circuit.h"
#include "simulation.h"
#include "timer.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <QCoreApplication>

#include <exception>
#include <iostream>

namespace logicsim {

///////////

//////////

/*
auto generate_layout() -> Layout {
    constexpr auto max_value = 300;
    auto editable_circuit = EditableCircuit {Layout {}};

    for (auto x : range(5, max_value, 5)) {
        for (auto y : range(5, max_value, 5)) {
            const auto definition = LogicItemDefinition {
                .element_type = ElementType::or_element,
                .input_count = 3,
                .output_count = 1,
            };

            editable_circuit.add_logic_item(definition, point_t {grid_t {x}, grid_t {y}},
                                            InsertionMode::insert_or_discard);

            editable_circuit.add_line_segments(point_t {grid_t {x + 2}, grid_t {y + 1}},
                                               point_t {grid_t {x + 4}, grid_t {y - 1}},
                                               LineInsertionType::horizontal_first,
                                               InsertionMode::insert_or_discard);

            editable_circuit.add_line_segments(point_t {grid_t {x + 3}, grid_t {y + 1}},
                                               point_t {grid_t {x + 5}, grid_t {y + 2}},
                                               LineInsertionType::vertical_first,
                                               InsertionMode::insert_or_discard);
        }
    }

    return editable_circuit.extract_layout();
}

auto render_single_layer(BLImage& img, const Layout& layout, unsigned int threads)
    -> void {
    CircuitContext {
        Context {.bl_image = img}
    }

    const auto info = BLContextCreateInfo {.threadCount = threads};
    auto ctx = BLContext {img, info};

    render_background(ctx, settings);
    // render_circuit(ctx, render_args_t {
    //                         .layout = layout,
    //                         .selection = Selection {},
    //                         .settings = settings,
    //                     });
    ctx.end();
}

auto benchmark_single_layer(const Layout& layout, int width, int height,
                            unsigned int threads) -> BLImage {
    auto img = BLImage {width, height, BL_FORMAT_PRGB32};
    const auto settings = OldRenderSettings {};

    for (auto _ [[maybe_unused]] : range(10)) {
        const auto t = Timer("single");
        for (auto __ [[maybe_unused]] : range(10)) {
            render_single_layer(img, layout, settings, threads);
        }
    }

    return img;
}

//
// multi layer
//

auto render_multi_layer(std::vector<BLImage>& images, const Layout& layout,
                        const OldRenderSettings& settings, unsigned int threads) -> void {
    const auto info = BLContextCreateInfo {.threadCount = threads};

    auto contexts = std::vector<BLContext> {};
    for (auto& img : images) {
        contexts.emplace_back(BLContext {img, info});
    }

    for (auto& ctx : std::ranges::subrange(contexts.begin() + 1, contexts.end())) {
        ctx.clearAll();
    }
    render_background(contexts.at(0), settings);
    render_layout(contexts.at(1), layout, settings);

    // finish rendering all layers
    for (auto& ctx : std::ranges::subrange(contexts.begin() + 1, contexts.end())) {
        ctx.end();
    }
    // compose layers
    for (auto& img : std::ranges::subrange(images.begin() + 1, images.end())) {
        contexts.at(0).blitImage(
            BLRect {
                0,
                0,
                gsl::narrow<double>(img.width()),
                gsl::narrow<double>(img.height()),
            },
            img);
    }
    contexts.at(0).end();
}

auto benchmark_multi_layer(const Layout& layout, int width, int height,
                           unsigned int threads) -> std::vector<BLImage> {
    const auto settings = OldRenderSettings {};
    const auto n = 2;

    auto images = std::vector<BLImage> {};
    for (auto _ [[maybe_unused]] : range(n)) {
        images.emplace_back(BLImage {width, height, BL_FORMAT_PRGB32});
    }

    for (auto _ [[maybe_unused]] : range(10)) {
        const auto t = Timer("multi");
        for (auto __ [[maybe_unused]] : range(10)) {
            render_multi_layer(images, layout, settings, threads);
        }
    }

    return images;
}


auto benchmark_layered_drawing() -> void {
    const int width = 3840;
    const int height = 2160;
    // const int width = 1920;
    // const int height = 1080;

    const unsigned int threads = 4;

    const auto layout = generate_layout();
    print(layout.format_stats());

    auto img_single = benchmark_single_layer(layout, width, height, threads);
    print();
    auto img_multi = benchmark_multi_layer(layout, width, height, threads);

    // write all images
    img_single.writeToFile(std::string {"main_single_layer.png"}.c_str());
    for (auto index : range(img_multi.size())) {
        img_multi.at(index).writeToFile(
            fmt::format("main_multi_layer_{}.png", index).c_str());
    }
}

*/

}  // namespace logicsim

auto main(int argc, char** argv) -> int {
    using namespace logicsim;
    const auto app [[maybe_unused]] = QCoreApplication {argc, argv};

    bool do_run = true;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    if (do_run) {
        try {
            auto timer = Timer {"Benchmark", Timer::Unit::ms, 3};
            // auto count = benchmark_simulation(6, 10, true);
            // auto count = logicsim::benchmark_simulation(BENCHMARK_DEFAULT_ELEMENTS,
            //                                            BENCHMARK_DEFAULT_EVENTS, true);

            auto count = logicsim::benchmark_line_renderer(100, true);
            fmt::print("count = {}\n", count);

        } catch (const std::exception& exc) {
            std::cerr << exc.what() << '\n';
            return -1;
        }
    }

    return 0;
}
