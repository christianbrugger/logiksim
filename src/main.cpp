
#include "benchmark/render_line_scene.h"
#include "default_element_definition.h"
#include "editable_circuit.h"
#include "logging.h"
#include "render_circuit.h"
#include "timer.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <exception>
#include <iostream>

namespace logicsim {

//

auto test_sanitized_render() {
    auto editable_circuit = EditableCircuit {};

    const auto definition = default_element_definition(LogicItemType::flipflop_jk);

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
        // render_background(circuit_ctx.ctx);
        // render_layout(circuit_ctx, layout);

        // draw_logic_item_base(ctx, layout, logicitem_id_t {0},
        // ElementDrawState::normal);

        // static constexpr auto input_labels = string_array<5> {"> C", "J", "K", "S",
        // "R"}; static constexpr auto output_labels = string_array<2> {"Q", "Q\u0305"};
        // draw_connector_labels(ctx, layout, logicitem_id_t {0},
        //                       ConnectorLabels {input_labels, output_labels},
        //                      ElementDrawState::normal);

        // draw_connector_label(ctx, point_t {5, 7}, orientation_t::left, "K",
        //                      ElementDrawState::normal);

        // ctx.text_cache.draw_text(ctx.bl_ctx, BLPoint {94, 126}, "K", 10.8,
        //                          defaults::color_black, HTextAlignment::right,
        //                           VTextAlignment::baseline);

        const auto font_files = get_default_font_locations();
        const auto faces = FontFaces {font_files};
        auto fonts = Fonts {faces};

        const auto font_size = 10.8f;
        fonts.regular.setSize(font_size);

        const auto shaped_text =
            HarfbuzzShapedText {"K", faces.regular.hb_font_face(), font_size};

        const auto codepoints = std::vector<uint32_t> {46};
        const auto placements = std::vector<BLGlyphPlacement> {
            BLGlyphPlacement {.placement = BLPointI {0, 0}, .advance = BLPointI {619, 0}},
        };

        auto glyph_run = BLGlyphRun {};
        glyph_run.size = std::min(codepoints.size(), placements.size());
        glyph_run.setGlyphData(codepoints.data());
        glyph_run.setPlacementData(placements.data());
        glyph_run.placementType = BL_GLYPH_PLACEMENT_TYPE_ADVANCE_OFFSET;

        print("shaped_text", shaped_text);

        ctx.bl_ctx.fillGlyphRun(BLPoint {87, 126}, fonts.regular, glyph_run,
                                defaults::color_black);

        ctx.end();

        ctx.bl_image.writeToFile(filename.c_str());
    }
    print("done");
}

}  // namespace logicsim

auto main() -> int {
    using namespace logicsim;

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
