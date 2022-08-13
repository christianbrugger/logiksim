
#include <iostream>

#include <benchmark/benchmark.h>

#include "circuit.h"


static void BM_StringCreationGlobber(benchmark::State& state) {
  for ([[maybe_unused]] auto _: state) {
    std::string empty_string;
	benchmark::DoNotOptimize(empty_string);
  }
}
BENCHMARK(BM_StringCreationGlobber); // NOLINT


static void BM_StringCopy(benchmark::State& state) {
  std::string x = "hello";
  for ([[maybe_unused]] auto _: state) {
    std::string copy(x);
	benchmark::DoNotOptimize(copy);
  }
}
BENCHMARK(BM_StringCopy); // NOLINT

BENCHMARK_MAIN(); // NOLINT
