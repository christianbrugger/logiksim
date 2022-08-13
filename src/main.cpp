
#include <iostream>

#include <benchmark/benchmark.h>


static void BM_StringCreationGlobber(benchmark::State& state) {
  for (auto _ : state) {
    std::string empty_string;
	benchmark::DoNotOptimize(empty_string);
  }
}
BENCHMARK(BM_StringCreationGlobber);


static void BM_StringCopy(benchmark::State& state) {
  std::string x = "hello";
  for (auto _ : state) {
    std::string copy(x);
	benchmark::DoNotOptimize(copy);
  }
}
BENCHMARK(BM_StringCopy);

BENCHMARK_MAIN();
