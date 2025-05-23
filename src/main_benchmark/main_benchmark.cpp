
// #define _DISABLE_VECTOR_ANNOTATION
// #define _DISABLE_STRING_ANNOTATION

#include "core/algorithm/range.h"
#include "core/benchmark/render_line_scene.h"
#include "core/benchmark/schematic_creation.h"
#include "core/benchmark/simulation_runtime.h"
#include "core/component/editable_circuit/selection_store.h"
#include "core/editable_circuit.h"
#include "core/element/logicitem/layout_logicitem.h"
#include "core/layout_info.h"
#include "core/logging.h"
#include "core/random/generator.h"
#include "core/random/layout_calculation_data.h"
#include "core/random/random_schematic.h"
#include "core/render/circuit/render_circuit.h"
#include "core/render/render_context.h"
#include "core/schematic_generation.h"
#include "core/simulation.h"
#include "core/vocabulary/color.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/logicitem_id.h"

#include <benchmark/benchmark.h>
#include <blend2d.h>
#include <gsl/gsl>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <numeric>
#include <ranges>
#include <vector>

namespace logicsim {

/*
static void BM_Benchmark_New_Selection(benchmark::State& state) {
    using namespace logicsim;

    const auto logicitem_id = logicitem_id_t {0};
    const auto registrar = SelectionRegistrar {};

    for ([[maybe_unused]] auto _ : state) {
        auto handle = registrar.get_handle();
        handle.value().add(logicitem_id);

        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(handle);
        benchmark::DoNotOptimize(handle);
    }
}

BENCHMARK(BM_Benchmark_New_Selection);  // NOLINT

static void BM_Benchmark_Reuse_Selection(benchmark::State& state) {
    using namespace logicsim;

    const auto logicitem_id = logicitem_id_t {0};
    const auto registrar = SelectionRegistrar {};
    auto handle = registrar.get_handle();

    for ([[maybe_unused]] auto _ : state) {
        handle.value().add(logicitem_id);
        benchmark::DoNotOptimize(handle);

        handle.value().remove_logicitem(logicitem_id);
        benchmark::DoNotOptimize(handle);
    }
}

BENCHMARK(BM_Benchmark_Reuse_Selection);  // NOLINT
*/

static void BM_Benchmark_Add_Element_Delete(benchmark::State& state) {
    using namespace logicsim;

    auto ec = EditableCircuit {};

    int x = 0;
    int y = 0;

    for ([[maybe_unused]] auto _ : state) {
        x += 5;
        if (x >= 10000) {
            y += 5;
            x = 0;
        }

        auto definition = LogicItemDefinition {
            .logicitem_type = LogicItemType::and_element,
            .input_count = connection_count_t {3},
            .output_count = connection_count_t {1},
            .orientation = orientation_t::right,
        };

        ec.add_logicitem(std::move(definition), point_t {grid_t {x}, grid_t {y}},
                         InsertionMode::insert_or_discard);
        // ec.delete_all(std::move(handle));
        benchmark::DoNotOptimize(ec);
    }
}

BENCHMARK(BM_Benchmark_Add_Element_Delete);  // NOLINT

static void BM_Benchmark_Input_Output_Valid(benchmark::State& state) {
    using namespace logicsim;

    for ([[maybe_unused]] auto _ : state) {
        auto res = is_input_output_count_valid(
            LogicItemType::and_element, connection_count_t {2}, connection_count_t {3});
        benchmark::DoNotOptimize(res);
    }
}

BENCHMARK(BM_Benchmark_Input_Output_Valid);  // NOLINT

namespace {
auto get_layout_test_data(std::size_t count) {
    auto rng = get_random_number_generator(0);

    auto data = std::vector<layout_calculation_data_t>(count);
    std::ranges::generate(data, [&] { return get_random_layout_calculation_data(rng); });
    return data;
}
}  // namespace

static void BM_Benchmark_Iter_SmallVector_Private(benchmark::State& state) {
    constexpr static auto N = 1024;

    auto data = get_layout_test_data(N);
    auto index = std::size_t {0};
    // auto sum = int64_t {0};

    auto buffer = std::array<point_t, 100>();

    for ([[maybe_unused]] auto _ : state) {
        index = (index + 1) % N;
        auto p_index = std::size_t {0};

        for (const auto point : element_body_points_base(data[index])) {
            buffer.at(p_index++) = point;
            // sum += int {point.x} + int {point.y};
        }

        benchmark::DoNotOptimize(buffer);
    }
    // print(sum);
}

BENCHMARK(BM_Benchmark_Iter_SmallVector_Private);  // NOLINT

static void BM_Benchmark_Graph_v2(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        using namespace logicsim;

        auto schematic = benchmark_schematic(1);

        benchmark::ClobberMemory();

        add_missing_placeholders(schematic);

        benchmark::DoNotOptimize(schematic);
    }
}

BENCHMARK(BM_Benchmark_Graph_v2);  // NOLINT

static void BM_Simulation_0(benchmark::State& state) {
    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        state.PauseTiming();

        auto rng = Rng {0};

        auto schematic = create_random_schematic(rng, 100);
        schematic = with_custom_delays(rng, schematic);
        add_missing_placeholders(schematic);

        // print(schematic);
        // for (auto element_id : element_ids(schematic)) {
        //     print_fmt(
        //         "{}, {}, history_length = {}, output_delays = {}, input_inverters =
        //         {}\n", element_id, schematic.element_type(element_id),
        //         schematic.history_length(element_id),
        //         schematic.output_delays(element_id),
        //         schematic.input_inverters(element_id));
        // }

        benchmark::DoNotOptimize(schematic);
        benchmark::ClobberMemory();

        state.ResumeTiming();

        count += benchmark_simulation(rng, std::move(schematic), 10'000);

        benchmark::DoNotOptimize(count);
        benchmark::ClobberMemory();
    }
    state.counters["Events"] =
        benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_Simulation_0);  // NOLINT

static void BM_Simulation_Inverter_Loop(benchmark::State& state) {
    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        state.PauseTiming();

        auto schematic = Schematic {};
        for (auto _1 [[maybe_unused]] : range(8)) {
            auto inverter = schematic.add_element(schematic::NewElement {
                .element_type = ElementType::buffer_element,
                .input_count = connection_count_t {1},
                .output_count = connection_count_t {1},
                .input_inverters = {false},
                .output_delays = {delay_t {3us}},
            });
            auto wire = schematic.add_element(schematic::NewElement {
                .element_type = ElementType::wire,
                .input_count = connection_count_t {1},
                .output_count = connection_count_t {1},
                .input_inverters = {true},
                .output_delays = {delay_t {1ns}},
            });
            const auto id_0 = connection_id_t {0};
            schematic.connect(output_t {inverter, id_0}, input_t {wire, id_0});
            schematic.connect(output_t {wire, id_0}, input_t {inverter, id_0});
        }

        benchmark::DoNotOptimize(schematic);
        benchmark::ClobberMemory();

        state.ResumeTiming();

        count += benchmark_simulation_metastable(std::move(schematic), 10'000);

        benchmark::DoNotOptimize(count);
        benchmark::ClobberMemory();
    }

    state.counters["Events"] =
        benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_Simulation_Inverter_Loop);  // NOLINT

static void BM_RenderScene_0(benchmark::State& state) {
    constexpr static auto save_image = false;  // to verify correctness

    const auto size = BLSizeI {1200, 1200};
    const auto scene = fill_line_scene(100);

    // setup rendering
    auto bl_image = BLImage {size.w, size.h, BL_FORMAT_PRGB32};
    const auto cache = ContextCache {};
    // TODO generate settings from bl_image, ...
    const auto settings = [&] {
        auto res = ContextRenderSettings {.thread_count = ThreadCount::synchronous};
        res.view_config.set_device_scale(12.);
        res.view_config.set_size(size);
        return res;
    }();

    render_to_image(bl_image, settings, cache,
                    [](Context& ctx) { ctx.bl_ctx.fillAll(defaults::color_white); });

    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        count += scene.total_wire_length_sum;

        render_to_image(bl_image, settings, cache, [&](Context& ctx) {
            render_simulation(ctx, scene.spatial_simulation);
        });

        benchmark::DoNotOptimize(bl_image);
        benchmark::ClobberMemory();

        if constexpr (save_image) {
            bl_image.writeToFile("google_benchmark_BM_RenderScene_0.png");
        }
    }

    state.counters["Events"] =
        benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RenderScene_0);  // NOLINT

}  // namespace logicsim

auto main(int argc, char** argv) -> int {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
