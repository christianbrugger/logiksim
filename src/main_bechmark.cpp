
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

namespace logicsim {

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
        schematic.validate(Schematic::validate_all);

        benchmark::DoNotOptimize(schematic);
    }
}

BENCHMARK(BM_Benchmark_Graph_v2);  // NOLINT

static void BM_Simulation_0(benchmark::State& state) {
    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        state.PauseTiming();

        boost::random::mt19937 rng {0};

        auto schematic = create_random_schematic(rng, 100);
        add_output_placeholders(schematic);
        // TODO fix bug and re-enable?
        // schematic.validate(Schematic::validate_all);

        benchmark::DoNotOptimize(schematic);
        benchmark::ClobberMemory();

        state.ResumeTiming();

        count += benchmark_simulation(rng, schematic, 10'000, false);

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
        for (auto _ [[maybe_unused]] : range(8)) {
            auto inverter = schematic.add_element(Schematic::ElementData {
                .element_type = ElementType::buffer_element,
                .input_count = 1,
                .output_count = 1,
                .input_inverters = {false},
                .output_delays = {delay_t {3us}},
            });
            auto wire = schematic.add_element(Schematic::ElementData {
                .element_type = ElementType::wire,
                .input_count = 1,
                .output_count = 1,
                .input_inverters = {true},
                .output_delays = {delay_t {1ns}},
            });
            const auto id_0 = connection_id_t {0};
            inverter.output(id_0).connect(wire.input(id_0));
            wire.output(id_0).connect(inverter.input(id_0));
        }
        schematic.validate(Schematic::validate_all);

        benchmark::DoNotOptimize(schematic);
        benchmark::ClobberMemory();

        state.ResumeTiming();

        count += benchmark_simulation_pure(schematic, 10'000, false);

        benchmark::DoNotOptimize(count);
        benchmark::ClobberMemory();
    }

    state.counters["Events"] =
        benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_Simulation_Inverter_Loop);  // NOLINT

static void BM_RenderScene_0(benchmark::State& state) {
    constexpr static auto save_image = false;  // to verify correctness

    auto scene = BenchmarkScene {};
    auto scene_count = fill_line_scene(scene, 100);

    auto context =
        CircuitContext {Context {.bl_image = BLImage {1200, 1200, BL_FORMAT_PRGB32},
                                 .settings = {.thread_count = 1}}};
    context.ctx.begin();

    {
        context.ctx.bl_ctx.fillAll(defaults::color_white);
        context.ctx.sync();
    }

    int64_t count = 0;
    for ([[maybe_unused]] auto _ : state) {
        count += scene_count;

        render_simulation(context, scene.layout, SimulationView {scene.simulation});
        context.ctx.sync();

        benchmark::DoNotOptimize(context.ctx.bl_image);
        benchmark::ClobberMemory();

        if constexpr (save_image) {
            context.ctx.bl_image.writeToFile("google_benchmark_BM_RenderScene_0.png");
        }
    }

    state.counters["Events"] =
        benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RenderScene_0);  // NOLINT

}  // namespace logicsim

BENCHMARK_MAIN();  // NOLINT
