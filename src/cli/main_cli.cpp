
#include "benchmark/render_line_scene.h"
#include "circuit_example.h"
#include "default_element_definition.h"
#include "editable_circuit.h"
#include "format/blend2d_type.h"
#include "geometry/scene.h"
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

auto render_overlay_simple(BLContext& bl_ctx) -> void {
    bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);

    const auto rect1 = BLRoundRect {81, 81, 55, 55, 5};
    const auto rect2 = BLRoundRect {171, 81, 55, 55, 5};
    const auto color = BLRgba32 {0, 128, 255, 96};

    bl_ctx.fillRoundRect(rect1, color);
    bl_ctx.fillRoundRect(rect2, color);
}

auto render_to_context(Context& ctx, ImageSurface& surface) -> void {
    // background
    ctx.bl_ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.bl_ctx.fillAll(defaults::color_white);

    // layer
    const auto rect = BLRectI {79, 79, 149, 59};
    render_layer(ctx, surface, rect,
                 [&](Context& layer_ctx) { render_overlay_simple(layer_ctx.bl_ctx); });
}

auto test_jit() -> void {
    // auto editable_circuit = load_example_with_logging(2);

    auto editable_circuit = EditableCircuit {};

    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::or_element,
        .input_count = connection_count_t {3},
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
        .output_inverters = {true},
    };
    editable_circuit.add_logicitem(definition, point_t {5, 5},
                                   InsertionMode::insert_or_discard);
    editable_circuit.add_logicitem(definition, point_t {10, 5},
                                   InsertionMode::insert_or_discard);
    visible_selection_select_all(editable_circuit);

    auto bl_image = BLImage {800, 600, BL_FORMAT_PRGB32};

    auto context_settings = ContextRenderSettings {};
    auto context_cache = ContextCache {cache_with_default_fonts()};
    auto context_surface = ImageSurface {};
    // auto render_config = WidgetRenderConfig {};

    context_settings.view_config.set_size(bl_image.size());

    render_to_image(bl_image, context_settings, context_cache,
                    [&](Context& ctx) { render_to_context(ctx, context_surface); });

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
