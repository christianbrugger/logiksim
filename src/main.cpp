
#include "benchmark/render_line_scene.h"
#include "default_element_definition.h"
#include "editable_circuit.h"
#include "logging.h"
#include "render_circuit.h"
#include "timer.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <QCoreApplication>

#include <exception>
#include <iostream>

namespace logicsim {

//

auto test_sanitized_render() {
    auto editable_circuit = EditableCircuit {};

    const auto definition = default_element_definition(LogicItemType::buffer_element);

    editable_circuit.add_logicitem(definition, point_t {5, 5},
                                   InsertionMode::insert_or_discard);

    // render_layout_to_file(editable_circuit.layout(), 500, 500, "render.png",
    //                      ViewConfig {});

    const auto layout = editable_circuit.layout();
    const auto filename = std::string {"render.png"};

    {
        auto circuit_ctx = CircuitContext {Context {
            .bl_image = BLImage {500, 500, BL_FORMAT_PRGB32},
            .settings = ContextRenderSettings {.thread_count = 0},
        }};
        auto& ctx = circuit_ctx.ctx;
        print("TEST", ctx.settings);

        ctx.begin();
        render_background(circuit_ctx.ctx);
        render_layout(circuit_ctx, layout);

        // draw_logic_item_base(ctx, layout, logicitem_id_t {0},
        // ElementDrawState::normal);

        ctx.end();

        ctx.bl_image.writeToFile(filename.c_str());
    }
    print("done");
}

}  // namespace logicsim

auto main(int argc, char** argv) -> int {
    using namespace logicsim;
    const auto app [[maybe_unused]] = QCoreApplication {argc, argv};

    bool do_run = true;

    test_sanitized_render();

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

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
