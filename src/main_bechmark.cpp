
// #define _DISABLE_VECTOR_ANNOTATION
// #define _DISABLE_STRING_ANNOTATION

#include "renderer.h"
#include "schematic.h"
#include "simulation.h"

#include <benchmark/benchmark.h>
#include <blend2d.h>
#include <boost/random/mersenne_twister.hpp>
#include <gsl/gsl>

#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

static void BM_Benchmark_Graph_v2(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        using namespace logicsim;

        auto circuit = benchmark_circuit(1);

        benchmark::ClobberMemory();

        circuit.validate();
        add_output_placeholders(circuit);
        circuit.validate(true);

        benchmark::DoNotOptimize(circuit);
    }
}

BENCHMARK(BM_Benchmark_Graph_v2);  // NOLINT

static void BM_Simulation_0(benchmark::State& state) {
    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        state.PauseTiming();

        boost::random::mt19937 rng {0};

        auto circuit = logicsim::create_random_circuit(rng, 100);
        logicsim::add_output_placeholders(circuit);
        circuit.validate(true);

        benchmark::DoNotOptimize(circuit);
        benchmark::ClobberMemory();

        state.ResumeTiming();

        count += logicsim::benchmark_simulation(rng, circuit, 10'000, false);

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
        render_circuit(ctx, scene.layout, scene.simulation);
        ctx.end();

        benchmark::DoNotOptimize(img);
        benchmark::ClobberMemory();
    }

    state.counters["Events"]
        = benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RenderScene_0);  // NOLINT

BENCHMARK_MAIN();  // NOLINT
