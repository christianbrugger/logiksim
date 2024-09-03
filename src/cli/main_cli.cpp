
#include "benchmark/render_line_scene.h"
#include "circuit_example.h"
#include "default_element_definition.h"
#include "editable_circuit.h"
#include "logging.h"
#include "render/managed_context.h"
#include "render_circuit.h"
#include "timer.h"
#include "vocabulary/widget_render_config.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <exception>
#include <iostream>

namespace logicsim {

auto render_to_context(Context& ctx, ImageSurface& surface,
                       const WidgetRenderConfig& render_config,
                       const EditableCircuit& editable_circuit,
                       bool show_size_handles) -> void {
    render_background(ctx);

    if (render_config.show_circuit) {
        const auto& target_layout = editable_circuit.layout();
        const auto& selection = editable_circuit.visible_selection();

        render_layout(ctx, surface, target_layout, selection);

        render_setting_handle(ctx, target_layout, selection);

        if (show_size_handles) {
            render_size_handles(ctx, target_layout, selection);
        }
    }
}

auto test_jit() -> void {
    auto editable_circuit = load_example_with_logging(2);
    visible_selection_select_all(editable_circuit);

    auto bl_image = BLImage {800, 600, BL_FORMAT_PRGB32};

    auto context_settings = ContextRenderSettings {};
    auto context_cache = ContextCache {cache_with_default_fonts()};
    auto context_surface = ImageSurface {};
    auto render_config = WidgetRenderConfig {};

    context_settings.view_config.set_size(bl_image.size());

    render_to_image(bl_image, context_settings, context_cache, [&](Context& ctx) {
        const auto show_size_handles = false;
        render_to_context(ctx, context_surface, render_config, editable_circuit,
                          show_size_handles);
    });

    bl_image.writeToFile("test_circuit.png");
}

}  // namespace logicsim

auto main() -> int {
    using namespace logicsim;

    bool do_run = false;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    test_jit();

    if (do_run) {
        try {
            auto timer = Timer {"Benchmark", Timer::Unit::ms, 3};
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
