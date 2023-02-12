
// #define _DISABLE_VECTOR_ANNOTATION
// #define _DISABLE_STRING_ANNOTATION

#include "range.h"
#include "renderer.h"
#include "schematic.h"
#include "simulation.h"

#include <absl/container/flat_hash_map.h>  // TODO remove
#include <ankerl/unordered_dense.h>        // TODO remove
#include <benchmark/benchmark.h>
#include <blend2d.h>
#include <boost/function_output_iterator.hpp>  // TODO remove
#include <boost/geometry.hpp>                  // TODO remove
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/unordered/unordered_flat_map.hpp>  // TODO remove
#include <gsl/gsl>

#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

static void BM_Tree_Vector(benchmark::State& state) {
    boost::random::mt19937 rng {0};
    std::vector<int> indices {};
    std::vector<int> values {};

    constexpr static auto i_max = 2048;
    std::ranges::generate_n(std::back_inserter(indices), i_max, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 999}(rng);
    });
    std::ranges::generate_n(std::back_inserter(values), 1000, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 1}(rng);
    });

    auto result = 0;
    auto i = 0;
    benchmark::ClobberMemory();
    for ([[maybe_unused]] auto _ : state) {
        result += values[indices[i % i_max]];
        ++i;
    }
    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_Tree_Vector);  // NOLINT

static void BM_Tree_Map_Ankerl(benchmark::State& state) {
    boost::random::mt19937 rng {0};
    std::vector<int> indices {};

    auto values = ankerl::unordered_dense::map<int, int> {};

    constexpr static auto i_max = 2048;
    std::ranges::generate_n(std::back_inserter(indices), i_max, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 999}(rng);
    });
    for (auto x : logicsim::range(1000)) {
        values[x] = boost::random::uniform_int_distribution<int> {0, 1}(rng);
    }

    auto result = 0;
    auto i = 0;
    benchmark::ClobberMemory();
    for ([[maybe_unused]] auto _ : state) {
        result += values[indices[i % i_max]];
        ++i;
    }
    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_Tree_Map_Ankerl);  // NOLINT

static void BM_Tree_Map_Abseil(benchmark::State& state) {
    boost::random::mt19937 rng {0};
    std::vector<int> indices {};

    auto values = absl::flat_hash_map<int, int> {};

    constexpr static auto i_max = 2048;
    std::ranges::generate_n(std::back_inserter(indices), i_max, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 999}(rng);
    });
    for (auto x : logicsim::range(1000)) {
        values[x] = boost::random::uniform_int_distribution<int> {0, 1}(rng);
    }

    auto result = 0;
    auto i = 0;
    benchmark::ClobberMemory();
    for ([[maybe_unused]] auto _ : state) {
        result += values[indices[i % i_max]];
        ++i;
    }
    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_Tree_Map_Abseil);  // NOLINT

static void BM_Tree_Map_Boost(benchmark::State& state) {
    boost::random::mt19937 rng {0};
    std::vector<int> indices {};

    auto values = boost::unordered::unordered_flat_map<int, int> {};

    constexpr static auto i_max = 2048;
    std::ranges::generate_n(std::back_inserter(indices), i_max, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 999}(rng);
    });
    for (auto x : logicsim::range(1000)) {
        values[x] = boost::random::uniform_int_distribution<int> {0, 1}(rng);
    }

    auto result = 0;
    auto i = 0;
    benchmark::ClobberMemory();
    for ([[maybe_unused]] auto _ : state) {
        result += values[indices[i % i_max]];
        ++i;
    }
    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_Tree_Map_Boost);  // NOLINT

static void BM_Tree_Sorted_Vector(benchmark::State& state) {
    boost::random::mt19937 rng {0};
    std::vector<int> indices {};
    constexpr static auto i_max = 2048;
    std::ranges::generate_n(std::back_inserter(indices), i_max, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 999}(rng);
    });

    auto keys = std::vector<int> {};
    auto values = std::vector<int> {};
    for (auto x : logicsim::range(1000)) {
        keys.push_back(x);
        values.push_back(boost::random::uniform_int_distribution<int> {0, 1}(rng));
    }

    auto result = 0;
    auto i = 0;
    benchmark::ClobberMemory();
    for ([[maybe_unused]] auto _ : state) {
        auto res = std::ranges::lower_bound(keys, indices[i % i_max]) - keys.begin();
        result += values[res];
        ++i;
    }
    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_Tree_Sorted_Vector);  // NOLINT

static void BM_Tree_Rtree_Query(benchmark::State& state) {
    boost::random::mt19937 rng {0};
    std::vector<int> indices {};
    constexpr static auto i_max = 2048;
    std::ranges::generate_n(std::back_inserter(indices), i_max, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 999}(rng);
    });

    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;

    using pt = bg::model::point<int, 1, bg::cs::cartesian>;
    using vt = std::pair<pt, int>;
    using tree_t = bgi::rtree<vt, bgi::rstar<16>>;

    auto tree = tree_t {};
    for (auto x : logicsim::range(1000)) {
        auto value = boost::random::uniform_int_distribution<int> {0, 1}(rng);
        tree.insert(vt {x, value});
    }

    auto result = 0;
    auto i = 0;
    benchmark::ClobberMemory();
    for ([[maybe_unused]] auto _ : state) {
        tree.query(bgi::contains(pt {indices[i % i_max]}),
                   boost::make_function_output_iterator(
                       [&result](const vt& found) { result += found.second; }));
        ++i;
    }
    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_Tree_Rtree_Query);  // NOLINT

static void BM_Tree_Rtree_Packing_Query(benchmark::State& state) {
    boost::random::mt19937 rng {0};
    std::vector<int> indices {};
    constexpr static auto i_max = 2048;
    std::ranges::generate_n(std::back_inserter(indices), i_max, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 999}(rng);
    });

    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;

    using pt = bg::model::point<int, 1, bg::cs::cartesian>;
    using vt = std::pair<pt, int>;
    using tree_t = bgi::rtree<vt, bgi::rstar<16>>;

    std::vector<vt> values {};
    for (auto x : logicsim::range(1000)) {
        auto value = boost::random::uniform_int_distribution<int> {0, 1}(rng);
        values.push_back(vt {x, value});
    }
    auto tree = tree_t {values};

    auto result = 0;
    auto i = 0;
    benchmark::ClobberMemory();
    for ([[maybe_unused]] auto _ : state) {
        tree.query(bgi::contains(pt {indices[i % i_max]}),
                   boost::make_function_output_iterator(
                       [&result](const vt& found) { result += found.second; }));
        ++i;
    }
    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_Tree_Rtree_Packing_Query);  // NOLINT

static void BM_Tree_Rtree_Count(benchmark::State& state) {
    boost::random::mt19937 rng {0};
    std::vector<int> indices {};
    constexpr static auto i_max = 2048;
    std::ranges::generate_n(std::back_inserter(indices), i_max, [&rng]() {
        return boost::random::uniform_int_distribution<int> {0, 999}(rng);
    });

    namespace bg = boost::geometry;
    namespace bgi = boost::geometry::index;

    using pt = bg::model::point<int, 1, bg::cs::cartesian>;
    using vt = std::pair<pt, int>;
    using tree_t = bgi::rtree<vt, bgi::rstar<16>>;

    auto tree = tree_t {};
    for (auto x : logicsim::range(1000)) {
        auto value = boost::random::uniform_int_distribution<int> {0, 1}(rng);
        if (value) {
            tree.insert(vt {x, value});
        }
    }

    auto result = 0;
    auto i = 0;
    benchmark::ClobberMemory();
    for ([[maybe_unused]] auto _ : state) {
        result += tree.count(pt {indices[i % i_max]});
        ++i;
    }
    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_Tree_Rtree_Count);  // NOLINT

//
// Real benchmarks
//

static void BM_Benchmark_Graph_v2(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        using namespace logicsim;

        auto schematic = benchmark_schematic(1);

        benchmark::ClobberMemory();

        schematic.validate();
        add_output_placeholders(schematic);
        schematic.validate(true);

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
        schematic.validate(true);

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
        render_circuit(ctx, scene.layout, scene.simulation, {.render_background = false});
        ctx.end();

        benchmark::DoNotOptimize(img);
        benchmark::ClobberMemory();
    }

    state.counters["Events"]
        = benchmark::Counter(gsl::narrow<double>(count), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RenderScene_0);  // NOLINT

BENCHMARK_MAIN();  // NOLINT
