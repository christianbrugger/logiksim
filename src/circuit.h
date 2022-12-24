#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

// investigations
// * iterator
// * std::span
// * write range(n), similar to python

#include "exceptions.h"
#include "random.h"

#include <boost/random/uniform_int_distribution.hpp>
#include <fmt/core.h>
#include <gsl/gsl>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/any_view.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/take_exactly.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>

#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

namespace logicsim {

enum class ElementType : uint8_t {
    placeholder,  // has no logic
    wire,
    inverter_element,
    and_element,
    or_element,
    xor_element
};

auto format(ElementType type) -> std::string;

using element_id_t = int32_t;
using connection_id_t = int32_t;
using connection_size_t = int8_t;

constexpr element_id_t null_element = -1;
constexpr connection_size_t null_connection = -1;

class Circuit {
    struct ElementData;
    struct ConnectionData;

   public:
    template <bool Const>
    class ElementTemplate;
    template <bool Const>
    class InputTemplate;
    template <bool Const>
    class OutputTemplate;

    using Element = ElementTemplate<false>;
    using ConstElement = ElementTemplate<true>;
    using Input = InputTemplate<false>;
    using ConstInput = InputTemplate<true>;
    using Output = OutputTemplate<false>;
    using ConstOutput = OutputTemplate<true>;

    [[nodiscard]] std::string format() const;

    [[nodiscard]] element_id_t element_count() const noexcept;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] bool is_element_id_valid(element_id_t element_id) const noexcept;
    [[nodiscard]] connection_id_t total_input_count() const noexcept;
    [[nodiscard]] connection_id_t total_output_count() const noexcept;

    [[nodiscard]] Element element(element_id_t element_id);
    [[nodiscard]] ConstElement element(element_id_t element_id) const;
    [[nodiscard]] auto elements()
        -> ranges::any_view<Element,
                            ranges::category::random_access | ranges::category::sized>;
    [[nodiscard]] auto elements() const
        -> ranges::any_view<ConstElement,
                            ranges::category::random_access | ranges::category::sized>;

    Element add_element(ElementType type, connection_size_t input_count,
                        connection_size_t output_count);
    auto clear() -> void;

    void validate(bool require_all_outputs_connected = false) const;

   private:
    static void validate_connection_data_(Circuit::ConnectionData connection_data);

    std::vector<ElementData> element_data_store_;
    std::vector<ConnectionData> output_data_store_;
    std::vector<ConnectionData> input_data_store_;
};

struct Circuit::ElementData {
    connection_id_t first_input_id;
    connection_id_t first_output_id;

    connection_size_t input_count;
    connection_size_t output_count;

    ElementType type;
};

struct Circuit::ConnectionData {
    element_id_t element_id {null_element};
    connection_size_t index {null_connection};
};

template <bool Const>
class Circuit::ElementTemplate {
    using CircuitType = std::conditional_t<Const, const Circuit, Circuit>;
    using ElementDataType = std::conditional_t<Const, const ElementData, ElementData>;

    friend ElementTemplate<!Const>;
    friend Circuit;
    explicit ElementTemplate(CircuitType &circuit, element_id_t element_id) noexcept;

   public:
    /// This constructor is not regarded as a copy constructor,
    //   so we preserve trivially copyable
    template <bool ConstOther>
    ElementTemplate(ElementTemplate<ConstOther> element) noexcept;

    template <bool ConstOther>
    bool operator==(ElementTemplate<ConstOther> other) const noexcept;

    [[nodiscard]] std::string format(bool with_connections = false) const;
    [[nodiscard]] std::string format_inputs() const;
    [[nodiscard]] std::string format_outputs() const;

    [[nodiscard]] CircuitType *circuit() const noexcept;
    [[nodiscard]] element_id_t element_id() const noexcept;

    [[nodiscard]] ElementType element_type() const;
    [[nodiscard]] connection_size_t input_count() const;
    [[nodiscard]] connection_size_t output_count() const;
    [[nodiscard]] connection_id_t first_input_id() const;
    [[nodiscard]] connection_id_t input_id(connection_size_t input_index) const;
    [[nodiscard]] connection_id_t first_output_id() const;
    [[nodiscard]] connection_id_t output_id(connection_size_t output_index) const;

    [[nodiscard]] InputTemplate<Const> input(connection_size_t input) const;
    [[nodiscard]] OutputTemplate<Const> output(connection_size_t output) const;

    [[nodiscard]] auto inputs() const
        -> ranges::any_view<InputTemplate<Const>,
                            ranges::category::random_access | ranges::category::sized>;
    [[nodiscard]] auto outputs() const
        -> ranges::any_view<OutputTemplate<Const>,
                            ranges::category::random_access | ranges::category::sized>;

   private:
    [[nodiscard]] ElementDataType &element_data_() const;

    CircuitType *circuit_;  // never to be null
    element_id_t element_id_;
};

template <bool Const>
class Circuit::InputTemplate {
    using CircuitType = std::conditional_t<Const, const Circuit, Circuit>;
    using ConnectionDataType
        = std::conditional_t<Const, const ConnectionData, ConnectionData>;

    /// This constructor is not regarded as a copy constructor,
    //   so we preserve trivially copyable
    friend InputTemplate<!Const>;
    friend ElementTemplate<Const>;
    explicit InputTemplate(CircuitType &circuit, element_id_t element_id,
                           connection_size_t input_index,
                           connection_id_t input_id) noexcept;

   public:
    template <bool ConstOther>
    InputTemplate(InputTemplate<ConstOther> input) noexcept;

    template <bool ConstOther>
    bool operator==(InputTemplate<ConstOther> other) const noexcept;

    [[nodiscard]] std::string format() const;
    [[nodiscard]] std::string format_connection() const;

    [[nodiscard]] CircuitType *circuit() const noexcept;
    [[nodiscard]] element_id_t element_id() const noexcept;
    [[nodiscard]] connection_size_t input_index() const noexcept;
    [[nodiscard]] connection_id_t input_id() const noexcept;

    [[nodiscard]] ElementTemplate<Const> element() const;
    [[nodiscard]] bool has_connected_element() const;
    [[nodiscard]] element_id_t connected_element_id() const;
    [[nodiscard]] connection_size_t connected_output_index() const;

    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] ElementTemplate<Const> connected_element() const;
    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] OutputTemplate<Const> connected_output() const;

    void clear_connection() const;
    template <bool ConstOther>
    void connect(OutputTemplate<ConstOther> output) const;

   private:
    [[nodiscard]] ConnectionDataType &connection_data_() const;

    CircuitType *circuit_;  // never to be null
    element_id_t element_id_;
    connection_size_t input_index_;
    connection_id_t input_id_;
};

template <bool Const>
class Circuit::OutputTemplate {
   private:
    using CircuitType = std::conditional_t<Const, const Circuit, Circuit>;
    using ConnectionDataType
        = std::conditional_t<Const, const ConnectionData, ConnectionData>;

    friend OutputTemplate<!Const>;
    friend ElementTemplate<Const>;
    explicit OutputTemplate(CircuitType &circuit, element_id_t element_id,
                            connection_size_t output_index,
                            connection_id_t output_id) noexcept;

   public:
    template <bool ConstOther>
    OutputTemplate(OutputTemplate<ConstOther> output) noexcept;

    template <bool ConstOther>
    bool operator==(OutputTemplate<ConstOther> other) const noexcept;

    [[nodiscard]] std::string format() const;
    [[nodiscard]] std::string format_connection() const;

    [[nodiscard]] CircuitType *circuit() const noexcept;
    [[nodiscard]] element_id_t element_id() const noexcept;
    [[nodiscard]] connection_size_t output_index() const noexcept;
    [[nodiscard]] connection_id_t output_id() const noexcept;

    [[nodiscard]] ElementTemplate<Const> element() const;
    [[nodiscard]] bool has_connected_element() const;
    [[nodiscard]] element_id_t connected_element_id() const;
    [[nodiscard]] connection_size_t connected_input_index() const;

    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] ElementTemplate<Const> connected_element() const;
    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] InputTemplate<Const> connected_input() const;

    void clear_connection() const;
    template <bool ConstOther>
    void connect(InputTemplate<ConstOther> input) const;

   private:
    [[nodiscard]] ConnectionDataType &connection_data_() const;

    CircuitType *circuit_;  // never to be null
    element_id_t element_id_;
    connection_size_t output_index_;
    connection_id_t output_id_;
};

// Template Instantiations

extern template class Circuit::ElementTemplate<true>;
extern template class Circuit::ElementTemplate<false>;

template <>
void Circuit::InputTemplate<false>::clear_connection() const;
extern template class Circuit::InputTemplate<true>;
extern template class Circuit::InputTemplate<false>;

template <>
void Circuit::OutputTemplate<false>::clear_connection() const;
extern template class Circuit::OutputTemplate<true>;
extern template class Circuit::OutputTemplate<false>;

}  // namespace logicsim

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::Circuit> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::ElementType> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::ElementType &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::Element> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::Element &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::ConstElement> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::ConstElement &obj,
                fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::Input> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::Input &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::ConstInput> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::ConstInput &obj,
                fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::Output> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::Output &obj, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Circuit::ConstOutput> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit::ConstOutput &obj,
                fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

//
// Free functions
//

namespace logicsim {

void add_output_placeholders(Circuit &circuit);

Circuit benchmark_circuit(int n_elements = 100);

namespace details {

template <std::uniform_random_bit_generator G>
void add_random_element(Circuit &circuit, G &rng) {
    boost::random::uniform_int_distribution<int8_t> element_dist {0, 2};
    boost::random::uniform_int_distribution<connection_size_t> connection_dist {1, 8};

    const auto element_type {element_dist(rng) == 0
                                 ? ElementType::xor_element
                                 : (element_dist(rng) == 1 ? ElementType::inverter_element
                                                           : ElementType ::wire)};

    const connection_size_t one {1};
    const connection_size_t input_count {
        element_type == ElementType::xor_element ? connection_dist(rng) : one};

    const connection_size_t output_count {
        element_type == ElementType::wire ? connection_dist(rng) : one};

    circuit.add_element(element_type, input_count, output_count);
}

template <std::uniform_random_bit_generator G>
void create_random_elements(Circuit &circuit, G &rng, int n_elements) {
    ranges::for_each(ranges::views::iota(0, n_elements),
                     [&](auto) { add_random_element(circuit, rng); });
}

template <std::uniform_random_bit_generator G>
void create_random_connections(Circuit &circuit, G &rng, double connection_ratio) {
    if (connection_ratio == 0) {
        return;
    }
    if (connection_ratio < 0 || connection_ratio > 1) [[unlikely]] {
        throw_exception("connection ratio needs to be between 0 and 1.");
    }

    auto all_inputs
        = circuit.elements()
          | ranges::views::transform([](auto element) { return element.inputs(); })
          | ranges::views::join | ranges::to_vector;
    auto all_outputs
        = circuit.elements()
          | ranges::views::transform([](auto element) { return element.outputs(); })
          | ranges::views::join | ranges::to_vector;

    shuffle(all_inputs, rng);
    shuffle(all_outputs, rng);

    auto n_connections = gsl::narrow<std::size_t>(
        std::round(connection_ratio
                   * std::min(ranges::size(all_inputs), ranges::size(all_outputs))));

    ranges::for_each(ranges::views::zip(all_inputs, all_outputs)
                         | ranges::views::take_exactly(n_connections),
                     [](const auto pair) {
                         const auto [input, output] = pair;
                         input.connect(output);
                     });
}

}  // namespace details

template <std::uniform_random_bit_generator G>
Circuit create_random_circuit(G &rng, int n_elements = 100,
                              double connection_ratio = 0.75) {
    Circuit circuit;
    details::create_random_elements(circuit, rng, n_elements);
    details::create_random_connections(circuit, rng, connection_ratio);

    return circuit;
}

}  // namespace logicsim

#endif
