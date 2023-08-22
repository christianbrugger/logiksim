
// #define _DISABLE_VECTOR_ANNOTATION
// #define _DISABLE_STRING_ANNOTATION

#include "editable_circuit/editable_circuit.h"
#include "editable_circuit/selection_registrar.h"
#include "range.h"
#include "render_benchmark.h"
#include "render_circuit.h"
#include "schematic.h"
#include "simulation.h"
#include "simulation_view.h"

#include <benchmark/benchmark.h>
#include <blend2d.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <gsl/gsl>

#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

static void BM_Benchmark_New_Selection(benchmark::State& state) {
    using namespace logicsim;

    const auto element_id = element_id_t {0};
    const auto registrar = SelectionRegistrar {};

    for ([[maybe_unused]] auto _ : state) {
        auto handle = registrar.get_handle();
        handle.value().add_logicitem(element_id);

        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(handle);
        benchmark::DoNotOptimize(handle);
    }
}

BENCHMARK(BM_Benchmark_New_Selection);  // NOLINT

static void BM_Benchmark_Reuse_Selection(benchmark::State& state) {
    using namespace logicsim;

    const auto element_id = element_id_t {0};
    const auto registrar = SelectionRegistrar {};
    auto handle = registrar.get_handle();

    for ([[maybe_unused]] auto _ : state) {
        handle.value().add_logicitem(element_id);
        benchmark::DoNotOptimize(handle);

        handle.value().remove_logicitem(element_id);
        benchmark::DoNotOptimize(handle);
    }
}

BENCHMARK(BM_Benchmark_Reuse_Selection);  // NOLINT

static void BM_Benchmark_Add_Element_Delete(benchmark::State& state) {
    using namespace logicsim;

    auto ec = EditableCircuit {Layout {}};

    int x = 0;
    int y = 0;

    for ([[maybe_unused]] auto _ : state) {
        x += 5;
        if (x >= 10000) {
            y += 5;
            x = 0;
        }

        const auto definition = LogicItemDefinition {
            .element_type = ElementType::and_element,
            .input_count = 3,
        };

        auto handle = ec.add_logic_item(definition, point_t {grid_t {x}, grid_t {y}},
                                        InsertionMode::insert_or_discard);
        // ec.delete_all(std::move(handle));
        benchmark::DoNotOptimize(handle);
    }
}

BENCHMARK(BM_Benchmark_Add_Element_Delete);  // NOLINT

static void BM_Benchmark_Graph_v2(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        using namespace logicsim;

        auto schematic = benchmark_schematic(1);

        benchmark::ClobberMemory();

        schematic.validate();
        add_output_placeholders(schematic);
        schematic.validate(logicsim::Schematic::validate_all);

        benchmark::DoNotOptimize(schematic);
    }
}

BENCHMARK(BM_Benchmark_Graph_v2);  // NOLINT

static void BM_Simulation_0(benchmark::State& state) {
    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        state.PauseTiming();

        boost::random::mt19937 rng {0};

        auto schematic = logicsim::create_random_schematic(rng, 100);
        logicsim::add_output_placeholders(schematic);
        // TODO fix bug and re-enable?
        // schematic.validate(logicsim::Schematic::validate_all);

        benchmark::DoNotOptimize(schematic);
        benchmark::ClobberMemory();

        state.ResumeTiming();

        count += logicsim::benchmark_simulation(rng, schematic, 10'000, false);

        benchmark::DoNotOptimize(count);
        benchmark::ClobberMemory();
    }
    state.counters["Events"] =
        benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_Simulation_0);  // NOLINT

static void BM_RenderScene_0(benchmark::State& state) {
    constexpr static auto save_image = false;  // to verify correctness

    logicsim::BenchmarkScene scene;
    auto scene_count = logicsim::fill_line_scene(scene, 100);

    BLImage img(1200, 1200, BL_FORMAT_PRGB32);
    {
        BLContext ctx(img);
        ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
        ctx.fillAll();
        ctx.end();
    }

    auto settings = logicsim::OldRenderSettings {};
    settings.view_config.set_size(img.width(), img.height());

    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        count += scene_count;

        BLContext ctx(img);
        render_simulation(ctx, scene.layout, logicsim::SimulationView {scene.simulation},
                          settings);

        ctx.end();

        benchmark::DoNotOptimize(img);
        benchmark::ClobberMemory();

        if constexpr (save_image) {
            BLImageCodec codec;
            codec.findByName("PNG");
            img.writeToFile("google_benchmark_BM_RenderScene_0.png", codec);
        }
    }

    state.counters["Events"] =
        benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RenderScene_0);  // NOLINT

BENCHMARK_MAIN();  // NOLINT
