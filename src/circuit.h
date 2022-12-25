#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

// investigations
// * iterator
// * std::span
// * write range(n), similar to python

#include "algorithm.h"
#include "exceptions.h"
#include "random.h"
#include "range.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <fmt/core.h>
#include <gsl/gsl>
#include <range/v3/range/concepts.hpp>

#include <concepts>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
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

    template <bool Const>
    class ElementViewTemplate;
    template <bool Const>
    class ElementIteratorTemplate;

    using ElementView = ElementViewTemplate<false>;
    using ConstElementView = ElementViewTemplate<true>;
    using ElementIterator = ElementIteratorTemplate<false>;
    using ConstElementIterator = ElementIteratorTemplate<true>;

    template <bool Const, bool IsInput>
    class ConnectionViewTemplate;
    template <bool Const, bool IsInput>
    class ConnectionIteratorTemplate;

    template <bool Const>
    using InputViewTemplate = ConnectionViewTemplate<Const, true>;
    template <bool Const>
    using InputIteratorTemplate = ConnectionIteratorTemplate<Const, true>;
    template <bool Const>
    using OutputViewTemplate = ConnectionViewTemplate<Const, false>;
    template <bool Const>
    using OutputIteratorTemplate = ConnectionIteratorTemplate<Const, false>;

    using InputView = InputViewTemplate<false>;
    using ConstInputView = InputViewTemplate<true>;
    using InputIterator = InputIteratorTemplate<false>;
    using ConstInputIterator = InputIteratorTemplate<true>;

    using OutputView = OutputViewTemplate<false>;
    using ConstOutputView = OutputViewTemplate<true>;
    using OutputIterator = OutputIteratorTemplate<false>;
    using ConstOutputIterator = OutputIteratorTemplate<true>;

    [[nodiscard]] std::string format() const;

    [[nodiscard]] element_id_t element_count() const noexcept;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] bool is_element_id_valid(element_id_t element_id) const noexcept;
    [[nodiscard]] connection_id_t total_input_count() const noexcept;
    [[nodiscard]] connection_id_t total_output_count() const noexcept;

    [[nodiscard]] Element element(element_id_t element_id);
    [[nodiscard]] ConstElement element(element_id_t element_id) const;
    [[nodiscard]] auto elements() noexcept -> ElementView;
    [[nodiscard]] auto elements() const noexcept -> ConstElementView;

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
class Circuit::ElementIteratorTemplate {
   public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    using value_type = Circuit::ElementTemplate<Const>;
    using difference_type = element_id_t;
    using pointer = value_type *;
    using reference = value_type &;

    using circuit_type = std::conditional_t<Const, const Circuit, Circuit>;

    // needs to be default constructable, so ElementView can become a range and view
    ElementIteratorTemplate() = default;

    [[nodiscard]] explicit constexpr ElementIteratorTemplate(
        circuit_type &circuit, element_id_t element_id) noexcept
        : circuit_(&circuit), element_id_(element_id) {}

    [[nodiscard]] constexpr auto operator*() const -> value_type {
        if (circuit_ == nullptr) [[unlikely]] {
            throw_exception("circuit cannot be null when dereferencing element iterator");
        }
        return circuit_->element(element_id_);
    }

    // Prefix increment
    constexpr auto operator++() noexcept -> Circuit::ElementIteratorTemplate<Const> & {
        ++element_id_;
        return *this;
    }

    // Postfix increment
    constexpr auto operator++(int) noexcept -> Circuit::ElementIteratorTemplate<Const> {
        auto tmp = *this;
        ++element_id_;
        return tmp;
    }

    [[nodiscard]] friend constexpr auto operator==(
        const Circuit::ElementIteratorTemplate<Const> &left,
        const Circuit::ElementIteratorTemplate<Const> &right) noexcept -> bool {
        return left.element_id_ >= right.element_id_;
    }

   private:
    circuit_type *circuit_ {};  // can be null
    element_id_t element_id_ {};
};

template <bool Const>
class Circuit::ElementViewTemplate {
   public:
    using iterator_type = Circuit::ElementIteratorTemplate<Const>;
    using circuit_type = std::conditional_t<Const, const Circuit, Circuit>;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    [[nodiscard]] explicit constexpr ElementViewTemplate(circuit_type &circuit) noexcept
        : circuit_(&circuit) {}

    [[nodiscard]] constexpr auto begin() const noexcept -> iterator_type {
        return iterator_type {*circuit_, 0};
    }

    [[nodiscard]] constexpr auto end() const noexcept -> iterator_type {
        return iterator_type {*circuit_, circuit_->element_count()};
    }

    [[nodiscard]] constexpr auto size() const noexcept -> element_id_t {
        return circuit_->element_count();
    }

    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        return circuit_->empty();
    }

   private:
    circuit_type *circuit_;  // never null
};

}  // namespace logicsim

template <bool Const>
inline constexpr bool
    std::ranges::enable_view<logicsim::Circuit::ElementViewTemplate<Const>>
    = true;

template <bool Const>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::Circuit::ElementViewTemplate<Const>>
    = true;

// TODO remove when not needed any more
template <bool Const>
inline constexpr bool ranges::enable_view<logicsim::Circuit::ElementViewTemplate<Const>>
    = true;

template <bool Const>
inline constexpr bool
    ranges::enable_borrowed_range<logicsim::Circuit::ElementViewTemplate<Const>>
    = true;

namespace logicsim {

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

    [[nodiscard]] auto inputs() const -> InputViewTemplate<Const>;
    [[nodiscard]] auto outputs() const -> OutputViewTemplate<Const>;

   private:
    [[nodiscard]] ElementDataType &element_data_() const;

    CircuitType *circuit_;  // never to be null
    element_id_t element_id_;
};

template <bool Const, bool IsInput>
class Circuit::ConnectionIteratorTemplate {
   public:
    std::optional<Circuit::ElementTemplate<Const>> element {};
    connection_size_t connection_id {};

    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    using value_type = std::conditional_t<IsInput, Circuit::InputTemplate<Const>,
                                          Circuit::OutputTemplate<Const>>;
    using difference_type = connection_size_t;
    using pointer = value_type *;
    using reference = value_type &;

    [[nodiscard]] constexpr auto operator*() const -> value_type {
        if (!element.has_value()) [[unlikely]] {
            throw_exception("iterator needs a valid element");
        }
        if constexpr (IsInput) {
            return element->input(connection_id);
        } else {
            return element->output(connection_id);
        }
    }

    // Prefix increment
    constexpr auto operator++() noexcept
        -> Circuit::ConnectionIteratorTemplate<Const, IsInput> & {
        ++connection_id;
        return *this;
    }

    // Postfix increment
    constexpr auto operator++(int) noexcept {
        auto tmp = *this;
        ++connection_id;
        return tmp;
    }

    [[nodiscard]] friend constexpr auto operator==(
        const Circuit::ConnectionIteratorTemplate<Const, IsInput> &left,
        const Circuit::ConnectionIteratorTemplate<Const, IsInput> &right) noexcept
        -> bool {
        return left.connection_id >= right.connection_id;
    }

    [[nodiscard]] friend constexpr auto operator-(
        const Circuit::ConnectionIteratorTemplate<Const, IsInput> &left,
        const Circuit::ConnectionIteratorTemplate<Const, IsInput> &right) noexcept
        -> difference_type {
        return left.connection_id - right.connection_id;
    }
};

template <bool Const, bool IsInput>
class Circuit::ConnectionViewTemplate {
   public:
    using iterator_type = Circuit::ConnectionIteratorTemplate<Const, IsInput>;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    [[nodiscard]] explicit constexpr ConnectionViewTemplate(
        ElementTemplate<Const> element) noexcept
        : element_(element) {}

    [[nodiscard]] constexpr auto begin() const -> iterator_type {
        return iterator_type {element_, 0};
    }

    [[nodiscard]] constexpr auto end() const -> iterator_type {
        return iterator_type {element_, size()};
    }

    [[nodiscard]] constexpr auto size() const -> connection_size_t {
        if constexpr (IsInput) {
            return element_.input_count();
        } else {
            return element_.output_count();
        }
    }

    [[nodiscard]] constexpr auto empty() const -> bool { return size() == 0; }

    [[nodiscard]] constexpr auto format() const -> std::string {
        // TODO try to pass member function directly
        // TODO test format-ranges compile time with res
        // TODO test boost join compile time
        auto format_single = [](value_type con) { return con.format_connection(); };
        auto res = transform_to_vector(begin(), end(), format_single);
        return fmt::format("[{}]", boost::join(res, ", "));
    }

   private:
    ElementTemplate<Const> element_;
};

}  // namespace logicsim

template <bool Const, bool IsInput>
inline constexpr bool
    std::ranges::enable_view<logicsim::Circuit::ConnectionViewTemplate<Const, IsInput>>
    = true;

template <bool Const, bool IsInput>
inline constexpr bool std::ranges::enable_borrowed_range<
    logicsim::Circuit::ConnectionViewTemplate<Const, IsInput>>
    = true;

// TODO remove when not needed any more
template <bool Const, bool IsInput>
inline constexpr bool
    ranges::enable_view<logicsim::Circuit::ConnectionViewTemplate<Const, IsInput>>
    = true;

template <bool Const, bool IsInput>
inline constexpr bool ranges::enable_borrowed_range<
    logicsim::Circuit::ConnectionViewTemplate<Const, IsInput>>
    = true;

namespace logicsim {

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
    for (auto _ [[maybe_unused]] : range(n_elements)) {
        add_random_element(circuit, rng);
    }
}

template <std::uniform_random_bit_generator G>
void create_random_connections(Circuit &circuit, G &rng, double connection_ratio) {
    if (connection_ratio == 0) {
        return;
    }
    if (connection_ratio < 0 || connection_ratio > 1) [[unlikely]] {
        throw_exception("connection ratio needs to be between 0 and 1.");
    }

    // collect inputs
    std::vector<Circuit::Input> all_inputs;
    all_inputs.reserve(circuit.total_input_count());
    for (auto element : circuit.elements()) {
        for (auto input : element.inputs()) {
            all_inputs.push_back(input);
        }
    }

    // collect outputs
    std::vector<Circuit::Output> all_outputs;
    all_outputs.reserve(circuit.total_output_count());
    for (auto element : circuit.elements()) {
        for (auto output : element.outputs()) {
            all_outputs.push_back(output);
        }
    }

    shuffle(all_inputs, rng);
    shuffle(all_outputs, rng);

    auto n_connections = gsl::narrow<std::size_t>(std::round(
        connection_ratio * std::min(std::size(all_inputs), std::size(all_outputs))));

    for (auto i [[maybe_unused]] : range(n_connections)) {
        all_inputs.at(i).connect(all_outputs.at(i));
    }
}

}  // namespace details

template <std::uniform_random_bit_generator G>
auto create_random_circuit(G &rng, int n_elements = 100, double connection_ratio = 0.75) {
    Circuit circuit;
    details::create_random_elements(circuit, rng, n_elements);
    details::create_random_connections(circuit, rng, connection_ratio);

    return circuit;
}

}  // namespace logicsim

#endif
