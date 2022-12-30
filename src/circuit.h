#ifndef LOGIKSIM_CIRCUIT_H
#define LOGIKSIM_CIRCUIT_H

#include <fmt/core.h>

#include <concepts>
#include <cstdint>
#include <iterator>
#include <optional>
#include <random>
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

// TODO use strong types
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

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto element_count() const noexcept -> element_id_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto is_element_id_valid(element_id_t element_id) const noexcept
        -> bool;
    [[nodiscard]] auto total_input_count() const noexcept -> connection_id_t;
    [[nodiscard]] auto total_output_count() const noexcept -> connection_id_t;

    [[nodiscard]] auto element(element_id_t element_id) -> Element;
    [[nodiscard]] auto element(element_id_t element_id) const -> ConstElement;
    [[nodiscard]] auto elements() noexcept -> ElementView;
    [[nodiscard]] auto elements() const noexcept -> ConstElementView;

    auto add_element(ElementType type, connection_size_t input_count,
                     connection_size_t output_count) -> Element;
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
    [[nodiscard]] explicit ElementIteratorTemplate(circuit_type &circuit,
                                                   element_id_t element_id) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    // Prefix increment
    auto operator++() noexcept -> Circuit::ElementIteratorTemplate<Const> &;
    // Postfix increment
    auto operator++(int) noexcept -> Circuit::ElementIteratorTemplate<Const>;

    [[nodiscard]] auto operator==(const ElementIteratorTemplate &right) const noexcept
        -> bool;
    [[nodiscard]] auto operator-(const Circuit::ElementIteratorTemplate<Const> &right)
        const noexcept -> difference_type;

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

    [[nodiscard]] explicit ElementViewTemplate(circuit_type &circuit) noexcept;

    [[nodiscard]] auto begin() const noexcept -> iterator_type;
    [[nodiscard]] auto end() const noexcept -> iterator_type;

    [[nodiscard]] auto size() const noexcept -> element_id_t;
    [[nodiscard]] auto empty() const noexcept -> bool;

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
// template <bool Const>
// inline constexpr bool
// ranges::enable_view<logicsim::Circuit::ElementViewTemplate<Const>>
//    = true;
//
// template <bool Const>
// inline constexpr bool
//    ranges::enable_borrowed_range<logicsim::Circuit::ElementViewTemplate<Const>>
//    = true;

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
    ElementTemplate(ElementTemplate<ConstOther> element) noexcept
        requires Const && (!ConstOther);

    template <bool ConstOther>
    auto operator==(ElementTemplate<ConstOther> other) const noexcept -> bool;

    [[nodiscard]] auto format(bool with_connections = false) const -> std::string;

    [[nodiscard]] auto circuit() const noexcept -> CircuitType *;
    [[nodiscard]] auto element_id() const noexcept -> element_id_t;

    [[nodiscard]] auto element_type() const -> ElementType;
    [[nodiscard]] auto input_count() const -> connection_size_t;
    [[nodiscard]] auto output_count() const -> connection_size_t;
    [[nodiscard]] auto first_input_id() const -> connection_id_t;
    [[nodiscard]] auto input_id(connection_size_t input_index) const -> connection_id_t;
    [[nodiscard]] auto first_output_id() const -> connection_id_t;
    [[nodiscard]] auto output_id(connection_size_t output_index) const -> connection_id_t;

    [[nodiscard]] auto input(connection_size_t input) const -> InputTemplate<Const>;
    [[nodiscard]] auto output(connection_size_t output) const -> OutputTemplate<Const>;

    [[nodiscard]] auto inputs() const -> InputViewTemplate<Const>;
    [[nodiscard]] auto outputs() const -> OutputViewTemplate<Const>;

   private:
    [[nodiscard]] auto element_data_() const -> ElementDataType &;

    CircuitType *circuit_;  // never null
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

    [[nodiscard]] auto operator*() const -> value_type;
    // Prefix increment
    auto operator++() noexcept -> Circuit::ConnectionIteratorTemplate<Const, IsInput> &;
    // Postfix increment
    auto operator++(int) noexcept -> ConnectionIteratorTemplate;

    [[nodiscard]] auto operator==(const ConnectionIteratorTemplate &right) const noexcept
        -> bool;
    [[nodiscard]] auto operator-(const ConnectionIteratorTemplate &right) const noexcept
        -> difference_type;
};

template <bool Const, bool IsInput>
class Circuit::ConnectionViewTemplate {
   public:
    using iterator_type = Circuit::ConnectionIteratorTemplate<Const, IsInput>;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    [[nodiscard]] explicit ConnectionViewTemplate(
        ElementTemplate<Const> element) noexcept;

    [[nodiscard]] auto begin() const -> iterator_type;
    [[nodiscard]] auto end() const -> iterator_type;

    [[nodiscard]] auto size() const -> connection_size_t;
    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto format() const -> std::string;

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
// template <bool Const, bool IsInput>
// inline constexpr bool
//    ranges::enable_view<logicsim::Circuit::ConnectionViewTemplate<Const, IsInput>>
//    = true;
//
// template <bool Const, bool IsInput>
// inline constexpr bool ranges::enable_borrowed_range<
//    logicsim::Circuit::ConnectionViewTemplate<Const, IsInput>>
//    = true;

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
    InputTemplate(InputTemplate<ConstOther> input) noexcept
        requires Const && (!ConstOther);

    template <bool ConstOther>
    auto operator==(InputTemplate<ConstOther> other) const noexcept -> bool;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_connection() const -> std::string;

    [[nodiscard]] auto circuit() const noexcept -> CircuitType *;
    [[nodiscard]] auto element_id() const noexcept -> element_id_t;
    [[nodiscard]] auto input_index() const noexcept -> connection_size_t;
    [[nodiscard]] auto input_id() const noexcept -> connection_id_t;

    [[nodiscard]] auto element() const -> ElementTemplate<Const>;
    [[nodiscard]] auto has_connected_element() const -> bool;
    [[nodiscard]] auto connected_element_id() const -> element_id_t;
    [[nodiscard]] auto connected_output_index() const -> connection_size_t;

    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] auto connected_element() const -> ElementTemplate<Const>;
    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] auto connected_output() const -> OutputTemplate<Const>;

    void clear_connection() const
        requires(!Const);
    template <bool ConstOther>
    void connect(OutputTemplate<ConstOther> output) const
        requires(!Const);

   private:
    [[nodiscard]] auto connection_data_() const -> ConnectionDataType &;

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
    OutputTemplate(OutputTemplate<ConstOther> output) noexcept
        requires Const && (!ConstOther);

    template <bool ConstOther>
    auto operator==(OutputTemplate<ConstOther> other) const noexcept -> bool;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_connection() const -> std::string;

    [[nodiscard]] auto circuit() const noexcept -> CircuitType *;
    [[nodiscard]] auto element_id() const noexcept -> element_id_t;
    [[nodiscard]] auto output_index() const noexcept -> connection_size_t;
    [[nodiscard]] auto output_id() const noexcept -> connection_id_t;

    [[nodiscard]] auto element() const -> ElementTemplate<Const>;
    [[nodiscard]] auto has_connected_element() const -> bool;
    [[nodiscard]] auto connected_element_id() const -> element_id_t;
    [[nodiscard]] auto connected_input_index() const -> connection_size_t;

    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] auto connected_element() const -> ElementTemplate<Const>;
    /// @throws if connection doesn't exists. Call has_connected_element to check for
    /// this.
    [[nodiscard]] auto connected_input() const -> InputTemplate<Const>;

    void clear_connection() const
        requires(!Const);
    template <bool ConstOther>
    void connect(InputTemplate<ConstOther> input) const
        requires(!Const);

   private:
    [[nodiscard]] auto connection_data_() const -> ConnectionDataType &;

    CircuitType *circuit_;  // never to be null
    element_id_t element_id_;
    connection_size_t output_index_;
    connection_id_t output_id_;
};

}  // namespace logicsim

//
// Formatters
//

template <>
struct fmt::formatter<logicsim::Circuit> {
    constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const logicsim::Circuit &obj, fmt::format_context &ctx) const
        -> format_context::iterator {
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

auto benchmark_circuit(int n_elements = 100) -> Circuit;

template <std::uniform_random_bit_generator G>
auto create_random_circuit(G &rng, int n_elements = 100, double connection_ratio = 0.75)
    -> Circuit;

}  // namespace logicsim

#endif
