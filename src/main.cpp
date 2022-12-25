
#include "circuit.h"
#include "simulation.h"

#include <absl/container/inlined_vector.h>
#include <boost/container/small_vector.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <folly/small_vector.h>

#include <exception>
#include <iostream>

template <typename T>
constexpr auto abseil_size() -> std::array<std::size_t, 33> {
    return {
        0,                                   //
        sizeof(absl::InlinedVector<T, 1>),   //
        sizeof(absl::InlinedVector<T, 2>),   //
        sizeof(absl::InlinedVector<T, 3>),   //
        sizeof(absl::InlinedVector<T, 4>),   //
        sizeof(absl::InlinedVector<T, 5>),   //
        sizeof(absl::InlinedVector<T, 6>),   //
        sizeof(absl::InlinedVector<T, 7>),   //
        sizeof(absl::InlinedVector<T, 8>),   //
        sizeof(absl::InlinedVector<T, 9>),   //
        sizeof(absl::InlinedVector<T, 10>),  //
        sizeof(absl::InlinedVector<T, 11>),  //
        sizeof(absl::InlinedVector<T, 12>),  //
        sizeof(absl::InlinedVector<T, 13>),  //
        sizeof(absl::InlinedVector<T, 14>),  //
        sizeof(absl::InlinedVector<T, 15>),  //
        sizeof(absl::InlinedVector<T, 16>),  //
        sizeof(absl::InlinedVector<T, 17>),  //
        sizeof(absl::InlinedVector<T, 18>),  //
        sizeof(absl::InlinedVector<T, 19>),  //
        sizeof(absl::InlinedVector<T, 20>),  //
        sizeof(absl::InlinedVector<T, 21>),  //
        sizeof(absl::InlinedVector<T, 22>),  //
        sizeof(absl::InlinedVector<T, 23>),  //
        sizeof(absl::InlinedVector<T, 24>),  //
        sizeof(absl::InlinedVector<T, 25>),  //
        sizeof(absl::InlinedVector<T, 26>),  //
        sizeof(absl::InlinedVector<T, 27>),  //
        sizeof(absl::InlinedVector<T, 28>),  //
        sizeof(absl::InlinedVector<T, 29>),  //
        sizeof(absl::InlinedVector<T, 30>),  //
        sizeof(absl::InlinedVector<T, 31>),  //
        sizeof(absl::InlinedVector<T, 32>),  //
    };
}

template <typename T>
constexpr auto folly_size() -> std::array<std::size_t, 33> {
    return {
        sizeof(folly::small_vector<T, 0>),   //
        sizeof(folly::small_vector<T, 1>),   //
        sizeof(folly::small_vector<T, 2>),   //
        sizeof(folly::small_vector<T, 3>),   //
        sizeof(folly::small_vector<T, 4>),   //
        sizeof(folly::small_vector<T, 5>),   //
        sizeof(folly::small_vector<T, 6>),   //
        sizeof(folly::small_vector<T, 7>),   //
        sizeof(folly::small_vector<T, 8>),   //
        sizeof(folly::small_vector<T, 9>),   //
        sizeof(folly::small_vector<T, 10>),  //
        sizeof(folly::small_vector<T, 11>),  //
        sizeof(folly::small_vector<T, 12>),  //
        sizeof(folly::small_vector<T, 13>),  //
        sizeof(folly::small_vector<T, 14>),  //
        sizeof(folly::small_vector<T, 15>),  //
        sizeof(folly::small_vector<T, 16>),  //
        sizeof(folly::small_vector<T, 17>),  //
        sizeof(folly::small_vector<T, 18>),  //
        sizeof(folly::small_vector<T, 19>),  //
        sizeof(folly::small_vector<T, 20>),  //
        sizeof(folly::small_vector<T, 21>),  //
        sizeof(folly::small_vector<T, 22>),  //
        sizeof(folly::small_vector<T, 23>),  //
        sizeof(folly::small_vector<T, 24>),  //
        sizeof(folly::small_vector<T, 25>),  //
        sizeof(folly::small_vector<T, 26>),  //
        sizeof(folly::small_vector<T, 27>),  //
        sizeof(folly::small_vector<T, 28>),  //
        sizeof(folly::small_vector<T, 29>),  //
        sizeof(folly::small_vector<T, 30>),  //
        sizeof(folly::small_vector<T, 31>),  //
        sizeof(folly::small_vector<T, 32>),  //
    };
}

template <typename T, typename Size>
constexpr auto folly_size_custom() -> std::array<std::size_t, 33> {
    return {
        sizeof(folly::small_vector<T, 0, Size>),   //
        sizeof(folly::small_vector<T, 1, Size>),   //
        sizeof(folly::small_vector<T, 2, Size>),   //
        sizeof(folly::small_vector<T, 3, Size>),   //
        sizeof(folly::small_vector<T, 4, Size>),   //
        sizeof(folly::small_vector<T, 5, Size>),   //
        sizeof(folly::small_vector<T, 6, Size>),   //
        sizeof(folly::small_vector<T, 7, Size>),   //
        sizeof(folly::small_vector<T, 8, Size>),   //
        sizeof(folly::small_vector<T, 9, Size>),   //
        sizeof(folly::small_vector<T, 10, Size>),  //
        sizeof(folly::small_vector<T, 11, Size>),  //
        sizeof(folly::small_vector<T, 12, Size>),  //
        sizeof(folly::small_vector<T, 13, Size>),  //
        sizeof(folly::small_vector<T, 14, Size>),  //
        sizeof(folly::small_vector<T, 15, Size>),  //
        sizeof(folly::small_vector<T, 16, Size>),  //
        sizeof(folly::small_vector<T, 17, Size>),  //
        sizeof(folly::small_vector<T, 18, Size>),  //
        sizeof(folly::small_vector<T, 19, Size>),  //
        sizeof(folly::small_vector<T, 20, Size>),  //
        sizeof(folly::small_vector<T, 21, Size>),  //
        sizeof(folly::small_vector<T, 22, Size>),  //
        sizeof(folly::small_vector<T, 23, Size>),  //
        sizeof(folly::small_vector<T, 24, Size>),  //
        sizeof(folly::small_vector<T, 25, Size>),  //
        sizeof(folly::small_vector<T, 26, Size>),  //
        sizeof(folly::small_vector<T, 27, Size>),  //
        sizeof(folly::small_vector<T, 28, Size>),  //
        sizeof(folly::small_vector<T, 29, Size>),  //
        sizeof(folly::small_vector<T, 30, Size>),  //
        sizeof(folly::small_vector<T, 31, Size>),  //
        sizeof(folly::small_vector<T, 32, Size>),  //
    };
}

template <typename T>
constexpr auto boost_size() -> std::array<std::size_t, 33> {
    return {
        sizeof(boost::container::small_vector<T, 0>),   //
        sizeof(boost::container::small_vector<T, 1>),   //
        sizeof(boost::container::small_vector<T, 2>),   //
        sizeof(boost::container::small_vector<T, 3>),   //
        sizeof(boost::container::small_vector<T, 4>),   //
        sizeof(boost::container::small_vector<T, 5>),   //
        sizeof(boost::container::small_vector<T, 6>),   //
        sizeof(boost::container::small_vector<T, 7>),   //
        sizeof(boost::container::small_vector<T, 8>),   //
        sizeof(boost::container::small_vector<T, 9>),   //
        sizeof(boost::container::small_vector<T, 10>),  //
        sizeof(boost::container::small_vector<T, 11>),  //
        sizeof(boost::container::small_vector<T, 12>),  //
        sizeof(boost::container::small_vector<T, 13>),  //
        sizeof(boost::container::small_vector<T, 14>),  //
        sizeof(boost::container::small_vector<T, 15>),  //
        sizeof(boost::container::small_vector<T, 16>),  //
        sizeof(boost::container::small_vector<T, 17>),  //
        sizeof(boost::container::small_vector<T, 18>),  //
        sizeof(boost::container::small_vector<T, 19>),  //
        sizeof(boost::container::small_vector<T, 20>),  //
        sizeof(boost::container::small_vector<T, 21>),  //
        sizeof(boost::container::small_vector<T, 22>),  //
        sizeof(boost::container::small_vector<T, 23>),  //
        sizeof(boost::container::small_vector<T, 24>),  //
        sizeof(boost::container::small_vector<T, 25>),  //
        sizeof(boost::container::small_vector<T, 26>),  //
        sizeof(boost::container::small_vector<T, 27>),  //
        sizeof(boost::container::small_vector<T, 28>),  //
        sizeof(boost::container::small_vector<T, 29>),  //
        sizeof(boost::container::small_vector<T, 30>),  //
        sizeof(boost::container::small_vector<T, 31>),  //
        sizeof(boost::container::small_vector<T, 32>),  //
    };
}

auto print_all_sizes() -> void {
    fmt::print("int8_t         {}\n", sizeof(int8_t));
    fmt::print("bool           {}\n", sizeof(bool));
    fmt::print("Abseil int8_t  {}\n", abseil_size<int8_t>());
    fmt::print("Abseil bool    {}\n", abseil_size<bool>());
    fmt::print("Folly int8_t   {}\n", folly_size<int8_t>());
    fmt::print("Folly bool     {}\n", folly_size<bool>());
    fmt::print("FollyC int8_t  {}\n", folly_size_custom<int8_t, uint8_t>());
    fmt::print("FollyC bool    {}\n", folly_size_custom<bool, uint8_t>());
    fmt::print("Boost int8_t   {}\n", boost_size<int8_t>());
    fmt::print("Boost bool     {}\n", boost_size<bool>());
}

auto main() -> int {
    using namespace logicsim;

    /// TODO consider: ios_base::sync_with_stdio(false);
    /// SL.io.10 in https://isocpp.github.io/CppCoreGuidelines/

    // print_all_sizes();

    try {
        // for ([[maybe_unused]] auto _ : ranges::views::iota(0, 10)) {
        auto count = benchmark_simulation(10, 10, true);
        fmt::print("count = {}\n", count);
        // }
        //

        boost::random::mt19937 rng {0};
        auto circuit = create_random_circuit(rng);

        fmt::print("\n\n\n\n");
        fmt::print("{}\n", ranges::size(circuit.elements()));
        // fmt::print("{}\n", circuit.elements()[2]);
        // auto test = circuit.element(0);
        // test.element_id();

        for (auto element : circuit.elements()) {
            fmt::print("{}\n", element.inputs().format());
            fmt::print("{}\n\n", element.format_inputs());
        }

    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return -1;
    }
    return 0;
}
