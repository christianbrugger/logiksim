
// #define _DISABLE_VECTOR_ANNOTATION
// #define _DISABLE_STRING_ANNOTATION

#include "editable_circuit/editable_circuit.h"
#include "editable_circuit/selection_registrar.h"
#include "range.h"
#include "renderer.h"
#include "schematic.h"
#include "simulation.h"

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
        auto handle = registrar.create_selection();
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
    auto handle = registrar.create_selection();

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

    auto ec = EditableCircuit {Circuit {Schematic {}, Layout {}}};

    int x = 0;
    int y = 0;

    for ([[maybe_unused]] auto _ : state) {
        x += 5;
        if (x >= 10000) {
            y += 5;
            x = 0;
        }

        auto handle = ec.add_standard_logic_item(ElementType::and_element, 3,
                                                 point_t {grid_t {x}, grid_t {y}},
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
    state.counters["Events"]
        = benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_Simulation_0);  // NOLINT

static void BM_RenderScene_0(benchmark::State& state) {
    logicsim::BenchmarkScene scene;
    auto scene_count = logicsim::fill_line_scene(scene, 100);

    BLImage img(1200, 1200, BL_FORMAT_PRGB32);
    {
        BLContext ctx(img);
        ctx.setFillStyle(BLRgba32(0xFFFFFFFFu));
        ctx.fillAll();
        ctx.end();
    }

    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        count += scene_count;

        BLContext ctx(img);
        logicsim::render_circuit(ctx, logicsim::render_args_t {
                                          .schematic = scene.schematic,
                                          .layout = scene.layout,
                                          .simulation = &scene.simulation,
                                      });
        ctx.end();

        benchmark::DoNotOptimize(img);
        benchmark::ClobberMemory();
    }

    state.counters["Events"]
        = benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RenderScene_0);  // NOLINT

BENCHMARK_MAIN();  // NOLINT
