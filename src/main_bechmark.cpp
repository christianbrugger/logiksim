
//#define _DISABLE_VECTOR_ANNOTATION
//#define _DISABLE_STRING_ANNOTATION

#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <numeric>
#include <random>
#include <functional>

#include <benchmark/benchmark.h>

#include "circuit.h"
// #include "simulation.h"


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

static void BM_Simulation(benchmark::State& state) {
	for ([[maybe_unused]] auto _ : state) {
		auto res = logicsim::benchmark_simulation();

		benchmark::DoNotOptimize(res);
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_Simulation); // NOLINT
*/

/* 
logicsim::CircuitGraph generate_graph(const int count) {
	using namespace logicsim;

	std::default_random_engine engine;
	std::uniform_int_distribution<int> distribution(0, 1);
	auto dice = std::bind(distribution, engine);

	CircuitGraph graph{};
	for (int i = 0; i < count; ++i) {
		graph.create_element(dice() ? ElementType::wire : ElementType::inverter_element,
			static_cast<connection_size_t>(dice() + 1), 
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

template <typename T>
class Base {
public:
	void test();

	template <bool Const>
	class Test;

private:
	T* t;
};

template <typename T>
void Base<T>::test() {

};

template <typename T>
template <bool Const>
class Base<T>::Test {

};


class Tree {
public:
	class Leaf;

	class Branch {
	public:
		Branch(Tree* tree, int branch_id)
			: tree_(tree), branch_id_(branch_id) {};

		Leaf leaf(int leaf_id) {
			return Leaf{ tree_, branch_id_, leaf_id };
		}

		float thickness() {
			return tree_->branch_thickness_[branch_id_];
		}

	private:
		Tree* tree_;
		int branch_id_;
	};

	class Leaf {
	public:
		Leaf(Tree* tree, int branch_id, int leaf_id)
			: tree_(tree), branch_id_(branch_id), leaf_id_(leaf_id) {};

		Branch branch() {
			return Branch{ tree_, branch_id_ };
		}

		float color() {
			return tree_->leaf_color_[branch_id_][leaf_id_];
		}
	private:
		Tree* tree_;
		int branch_id_;
		int leaf_id_;
	};

	Branch branch(int branch_id) {
		return Branch{ this, branch_id };
	}

//	Branch branch(int branch_id) const {
//		return Branch{ this, branch_id };
//	}
private:
	std::vector<float> branch_thickness_{ 0.5 };
	std::vector<std::vector<float>> leaf_color_{ {0.2, 0.4} };
};


void demo() {
	Tree tree;
	Tree::Branch branch = tree.branch(0);
	Tree::Leaf leaf = branch.leaf(1);

	std::cout << "Branch Thickness " << branch.thickness() << '\n';
	std::cout << "Leaf Color " << leaf.color() << '\n';
	std::cout << "Branch Thickness " << leaf.branch().thickness() << '\n';
}

/*
void demo_const() {
	const Tree tree;
	Tree::Branch branch = tree.branch(0);
	Tree::Leaf leaf = branch.leaf(1);

	std::cout << "Branch Thickness " << branch.thickness() << '\n';
	std::cout << "Leaf Color " << leaf.color() << '\n';
	std::cout << "Branch Thickness " << leaf.branch().thickness() << '\n';
}
*/

static void BM_Benchmark_Graph_v2(benchmark::State& state) {
	for ([[maybe_unused]] auto _ : state) {
		using namespace logicsim;

		auto circuit = benchmark_circuit(10'000);

		benchmark::ClobberMemory();

		create_placeholders(circuit);

		benchmark::DoNotOptimize(circuit);

		const Circuit circuit2 = circuit;

		// Base<int>::Test<true> abc;

		demo();
	}
}
BENCHMARK(BM_Benchmark_Graph_v2); // NOLINT






BENCHMARK_MAIN(); // NOLINT
