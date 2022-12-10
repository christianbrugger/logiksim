
// #define _DISABLE_VECTOR_ANNOTATION
// #define _DISABLE_STRING_ANNOTATION

#include "circuit.h"
#include "simulation.h"

#include <benchmark/benchmark.h>

#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

/*
class value_t {
        [[maybe_unused]] int a;
};

static void BM_VectorPointer(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                static constexpr int count = 1'000;
                std::vector<std::unique_ptr<value_t>> vec;
                vec.resize(count);

                for (int i = 0; i < count; ++i) {
                        vec[i] = std::make_unique<value_t>();
                }
                benchmark::DoNotOptimize(vec.data());
                benchmark::ClobberMemory();
        }
}
BENCHMARK(BM_VectorPointer); // NOLINT


static void BM_ArrayDirect(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                static constexpr int count = 1'000;
                std::vector<value_t> vec;
                vec.resize(count);

                benchmark::DoNotOptimize(vec.data());
                benchmark::ClobberMemory();
        }
}
BENCHMARK(BM_ArrayDirect); // NOLINT



static void BM_VectorFill(benchmark::State& state) {
        for ([[maybe_unused]] auto _: state) {
                static constexpr int count = 1'000;
                std::vector<int> vec;
                vec.resize(count);

                for (int i = 0; i < count; ++i) {
                        vec[i] = i;
                }
                benchmark::DoNotOptimize(vec.data());
                benchmark::ClobberMemory();
        }
}
BENCHMARK(BM_VectorFill); // NOLINT


static void BM_ArrayFill(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                static constexpr int count = 1'000;
                std::array<int, count> vec;

                for (int i = 0; i < count; ++i) {
                        vec[i] = i;
                }
                benchmark::DoNotOptimize(vec.data());
                benchmark::ClobberMemory();
        }
}
BENCHMARK(BM_ArrayFill); // NOLINT


static void BM_ArrayFillForEach(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                static constexpr int count = 1'000;
                std::array<int, count> vec;

                int i = 0;
                for (int& val: vec) {
                        val = i++;
                }
                benchmark::DoNotOptimize(vec.data());
                benchmark::ClobberMemory();
        }
}
BENCHMARK(BM_ArrayFillForEach); // NOLINT


static void BM_ArrayFillStd(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                static constexpr int count = 1'000;
                std::array<int, count> vec;

                std::iota(std::begin(vec), std::end(vec), 0);
                benchmark::DoNotOptimize(vec.data());
                benchmark::ClobberMemory();
        }
}
BENCHMARK(BM_ArrayFillStd); // NOLINT
*/

/*
static void BM_GraphEmpty(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                auto res = logicsim::benchmark_graph<logicsim::CircuitGraph>();

                benchmark::DoNotOptimize(res);
                benchmark::ClobberMemory();
        }
}
BENCHMARK(BM_GraphEmpty); // NOLINT


*/

/*
logicsim::CircuitGraph generate_graph(const int count) {
        using namespace logicsim;

        std::default_random_engine engine;
        std::uniform_int_distribution<int> distribution(0, 1);
        auto dice = std::bind(distribution, engine);

        CircuitGraph graph{};
        for (int i = 0; i < count; ++i) {
                graph.add_element(dice() ? ElementType::wire :
ElementType::inverter_element, static_cast<connection_size_t>(dice() + 1),
                        static_cast<connection_size_t>(dice() + 1)
                );
        }
        return graph;
}


static void BM_Loop_Manually(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                using namespace logicsim;

                CircuitGraph graph = generate_graph(1'000);

                benchmark::DoNotOptimize(graph);
                benchmark::ClobberMemory();
        }
}
BENCHMARK(BM_Loop_Manually); // NOLINT


static void BM_Loop_Manually_2(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                using namespace logicsim;

                CircuitGraph graph = generate_graph(1'000);

                benchmark::ClobberMemory();

                int sum = 0;

                for (int i = 0; i < 10; ++i) {
                        for (auto element : graph.elements()) {
                                auto type = graph.get_type(element);
                                sum += type == ElementType::wire ? 1 : 2;
                                sum += graph.get_input_count(element);
                                sum += graph.get_output_count(element);
                        }
                        benchmark::ClobberMemory();
                }

                benchmark::DoNotOptimize(sum);
        }
}
BENCHMARK(BM_Loop_Manually_2); // NOLINT


static void BM_Loop_Manually_3(benchmark::State& state) {
        for ([[maybe_unused]] auto _ : state) {
                using namespace logicsim;

                CircuitGraph graph = generate_graph(1'000);

                benchmark::ClobberMemory();

                int sum = 0;

                for (int i = 0; i < 10; ++i) {
                        for (auto element : graph.elements()) {
                                auto obj = CircuitElement(graph, element);

                                auto type = obj.get_type();
                                sum += type == ElementType::wire ? 1 : 2;
                                sum += obj.get_input_count();
                                sum += obj.get_output_count();
                        }
                        benchmark::ClobberMemory();
                }

                benchmark::DoNotOptimize(sum);
        }
}
BENCHMARK(BM_Loop_Manually_3); // NOLINT
*/

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
    for ([[maybe_unused]] auto _ : state) {
        auto res = logicsim::benchmark_simulation(0, 1000);

        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_Simulation_0);  // NOLINT

static void BM_Simulation_1(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto res = logicsim::benchmark_simulation(1, 1000);

        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_Simulation_1);  // NOLINT

static void BM_Simulation_2(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        auto res = logicsim::benchmark_simulation(2, 1000);

        benchmark::DoNotOptimize(res);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(BM_Simulation_2);  // NOLINT

BENCHMARK_MAIN();  // NOLINT
