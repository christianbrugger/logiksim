
//#define _DISABLE_VECTOR_ANNOTATION
//#define _DISABLE_STRING_ANNOTATION

#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <numeric>

#include <benchmark/benchmark.h>

#include "circuit.h"
#include "simulation.h"


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

static void BM_GraphEmpty(benchmark::State& state) {
	for ([[maybe_unused]] auto _ : state) {
		auto res = logicsim::benchmark_graph<logicsim::CircuitGraph>();

		benchmark::DoNotOptimize(res);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_GraphEmpty); // NOLINT

static void BM_Simulation(benchmark::State& state) {
	for ([[maybe_unused]] auto _ : state) {
		auto res = logicsim::benchmark_simulation();

		benchmark::DoNotOptimize(res);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_Simulation); // NOLINT


BENCHMARK_MAIN(); // NOLINT
