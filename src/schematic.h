#ifndef LOGIKSIM_SCHEMATIC_H
#define LOGIKSIM_SCHEMATIC_H

#include "format.h"
#include "vocabulary.h"

#include <fmt/core.h>
#include <folly/small_vector.h>

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

class Schematic {
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

    struct defaults {
        constexpr static delay_t button_delay {10ns};
        constexpr static delay_t standard_delay {3us};
        constexpr static delay_t clock_generator_delay {1ms};
        constexpr static delay_t wire_delay_per_distance {1us};

        constexpr static delay_t no_history {0ns};
    };

    explicit constexpr Schematic() = default;
    explicit Schematic(circuit_id_t circuit_id);

    auto clear() -> void;
    auto swap(Schematic &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto circuit_id() const noexcept -> circuit_id_t;
    [[nodiscard]] auto element_count() const noexcept -> std::size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto is_element_id_valid(element_id_t element_id) const noexcept
        -> bool;
    // TODO rename to total_...
    [[nodiscard]] auto input_count() const noexcept -> std::size_t;
    [[nodiscard]] auto output_count() const noexcept -> std::size_t;

    [[nodiscard]] auto element(element_id_t element_id) -> Element;
    [[nodiscard]] auto element(element_id_t element_id) const -> ConstElement;
    [[nodiscard]] auto elements() noexcept -> ElementView;
    [[nodiscard]] auto elements() const noexcept -> ConstElementView;

    [[nodiscard]] auto input(connection_t connection) -> Input;
    [[nodiscard]] auto input(connection_t connection) const -> ConstInput;
    [[nodiscard]] auto output(connection_t connection) -> Output;
    [[nodiscard]] auto output(connection_t connection) const -> ConstOutput;

    struct ElementData {
        ElementType element_type {ElementType::inverter_element};
        std::size_t input_count {0};
        std::size_t output_count {0};

        circuit_id_t circuit_id {null_circuit};
        logic_small_vector_t input_inverters {};
        std::vector<delay_t> output_delays {};
        delay_t history_length = defaults::no_history;
    };

    auto add_element(ElementData &&data) -> Element;
    // swaps the element with last one and deletes it
    auto swap_and_delete_element(element_id_t element_id) -> element_id_t;
    auto swap_elements(element_id_t element_id_0, element_id_t element_id_1) -> void;

    struct ValidationSettings {
        bool require_all_outputs_connected {false};
        bool require_all_placeholders_connected {false};
    };

    constexpr static auto validate_basic = ValidationSettings {false, false};
    constexpr static auto validate_all = ValidationSettings {true, true};

    void validate(ValidationSettings settings = validate_basic) const;

   private:
    auto swap_element_data(element_id_t element_id_1, element_id_t element_id_2,
                           bool update_connections) -> void;
    auto update_swapped_connections(element_id_t new_element_id,
                                    element_id_t old_element_id) -> void;
    auto delete_last_element(bool clear_connections) -> void;

    // output_delays type
    using policy = folly::small_vector_policy::policy_size_type<uint32_t>;
    using output_delays_t = folly::small_vector<delay_t, 5, policy>;
    static_assert(sizeof(output_delays_t) == 24);

    // TODO use connection_id_t as counter
    using connection_vector_t = folly::small_vector<connection_t, 3>;
    static_assert(sizeof(connection_t) == 8);
    static_assert(sizeof(connection_vector_t) == 32);

    std::vector<ElementType> element_types_ {};
    std::vector<circuit_id_t> sub_circuit_ids_ {};
    std::vector<connection_vector_t> input_connections_ {};
    std::vector<connection_vector_t> output_connections_ {};
    std::vector<logic_small_vector_t> input_inverters_ {};
    std::vector<output_delays_t> output_delays_ {};
    std::vector<delay_t> history_lengths_ {};

    std::size_t input_count_ {0};
    std::size_t output_count_ {0};
    circuit_id_t circuit_id_ {0};
};

auto swap(Schematic &a, Schematic &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::Schematic &a, logicsim::Schematic &b) noexcept -> void;

namespace logicsim {

template <bool IsInput, typename T>
auto to_connection(T &&schematic, connection_t connection_data)
    requires std::same_as<std::remove_cvref_t<T>, Schematic>
{
    if constexpr (IsInput) {
        return schematic.input(connection_data);
    } else {
        return schematic.output(connection_data);
    }
}

template <class T>
concept ElementOrConnection = std::convertible_to<T, Schematic::ConstElement>
                              || std::convertible_to<T, Schematic::ConstInput>
                              || std::convertible_to<T, Schematic::ConstOutput>;

template <bool Const>
class Schematic::ElementIteratorTemplate {
   public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    using value_type = Schematic::ElementTemplate<Const>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    using schematic_type = std::conditional_t<Const, const Schematic, Schematic>;

    // needs to be default constructable, so ElementView can become a range and view
    ElementIteratorTemplate() = default;
    [[nodiscard]] explicit ElementIteratorTemplate(schematic_type &schematic,
                                                   element_id_t element_id) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    // Prefix increment
    auto operator++() noexcept -> Schematic::ElementIteratorTemplate<Const> &;
    // Postfix increment
    auto operator++(int) noexcept -> Schematic::ElementIteratorTemplate<Const>;

    [[nodiscard]] auto operator==(const ElementIteratorTemplate &right) const noexcept
        -> bool;
    [[nodiscard]] auto operator-(const Schematic::ElementIteratorTemplate<Const> &right)
        const noexcept -> difference_type;

   private:
    schematic_type *schematic_ {};  // can be null, because default constructable
    element_id_t element_id_ {};
};

template <bool Const>
class Schematic::ElementViewTemplate {
   public:
    using iterator_type = Schematic::ElementIteratorTemplate<Const>;
    using schematic_type = std::conditional_t<Const, const Schematic, Schematic>;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    [[nodiscard]] explicit ElementViewTemplate(schematic_type &schematic) noexcept;

    [[nodiscard]] auto begin() const noexcept -> iterator_type;
    [[nodiscard]] auto end() const noexcept -> iterator_type;

    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;

   private:
    gsl::not_null<schematic_type *> schematic_;
};

}  // namespace logicsim

template <bool Const>
inline constexpr bool
    std::ranges::enable_view<logicsim::Schematic::ElementViewTemplate<Const>>
    = true;

template <bool Const>
inline constexpr bool
    std::ranges::enable_borrowed_range<logicsim::Schematic::ElementViewTemplate<Const>>
    = true;

namespace logicsim {

template <bool Const>
class Schematic::ElementTemplate {
    using SchematicType = std::conditional_t<Const, const Schematic, Schematic>;

    friend ElementTemplate<!Const>;
    friend Schematic;
    explicit ElementTemplate(SchematicType &schematic, element_id_t element_id) noexcept;

   public:
    /// This constructor is not regarded as a copy constructor,
    //   we preserve trivially copyable
    template <bool ConstOther>
    // NOLINTNEXTLINE(google-explicit-constructor)
    ElementTemplate(ElementTemplate<ConstOther> element) noexcept
        requires Const && (!ConstOther);

    [[nodiscard]] operator element_id_t() const noexcept;

    template <bool ConstOther>
    auto operator==(ElementTemplate<ConstOther> other) const noexcept -> bool;

    [[nodiscard]] auto format(bool with_connections = false) const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> SchematicType &;
    [[nodiscard]] auto element_id() const noexcept -> element_id_t;

    [[nodiscard]] auto element_type() const -> ElementType;
    [[nodiscard]] auto is_unused() const -> bool;
    [[nodiscard]] auto is_placeholder() const -> bool;
    [[nodiscard]] auto is_wire() const -> bool;
    [[nodiscard]] auto is_logic_item() const -> bool;
    [[nodiscard]] auto is_sub_circuit() const -> bool;

    [[nodiscard]] auto sub_circuit_id() const -> circuit_id_t;
    [[nodiscard]] auto input_inverters() const -> const logic_small_vector_t &;
    [[nodiscard]] auto output_delays() const -> const output_delays_t &;
    [[nodiscard]] auto history_length() const -> delay_t;

    [[nodiscard]] auto input_count() const -> std::size_t;
    [[nodiscard]] auto output_count() const -> std::size_t;

    [[nodiscard]] auto input(connection_id_t input) const -> InputTemplate<Const>;
    [[nodiscard]] auto output(connection_id_t output) const -> OutputTemplate<Const>;

    [[nodiscard]] auto inputs() const -> InputViewTemplate<Const>;
    [[nodiscard]] auto outputs() const -> OutputViewTemplate<Const>;

    auto clear_all_connection() const -> void
        requires(!Const);

   private:
    gsl::not_null<SchematicType *> schematic_;
    element_id_t element_id_;
};

template <bool Const, bool IsInput>
class Schematic::ConnectionIteratorTemplate {
   public:
    std::optional<Schematic::ElementTemplate<Const>> element {};
    connection_id_t connection_id {};

    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    using value_type = std::conditional_t<IsInput, Schematic::InputTemplate<Const>,
                                          Schematic::OutputTemplate<Const>>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    [[nodiscard]] auto operator*() const -> value_type;
    // Prefix increment
    auto operator++() noexcept -> Schematic::ConnectionIteratorTemplate<Const, IsInput> &;
    // Postfix increment
    auto operator++(int) noexcept -> ConnectionIteratorTemplate;

    [[nodiscard]] auto operator==(const ConnectionIteratorTemplate &right) const noexcept
        -> bool;
    [[nodiscard]] auto operator-(const ConnectionIteratorTemplate &right) const noexcept
        -> difference_type;
};

template <bool Const, bool IsInput>
class Schematic::ConnectionViewTemplate {
   public:
    using iterator_type = Schematic::ConnectionIteratorTemplate<Const, IsInput>;

    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;

    [[nodiscard]] explicit ConnectionViewTemplate(
        ElementTemplate<Const> element) noexcept;

    [[nodiscard]] auto begin() const -> iterator_type;
    [[nodiscard]] auto end() const -> iterator_type;

    [[nodiscard]] auto size() const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;

    [[nodiscard]] auto format() const -> std::string;

   private:
    ElementTemplate<Const> element_;
};

}  // namespace logicsim

template <bool Const, bool IsInput>
inline constexpr bool
    std::ranges::enable_view<logicsim::Schematic::ConnectionViewTemplate<Const, IsInput>>
    = true;

template <bool Const, bool IsInput>
inline constexpr bool std::ranges::enable_borrowed_range<
    logicsim::Schematic::ConnectionViewTemplate<Const, IsInput>>
    = true;

namespace logicsim {

template <bool Const>
class Schematic::InputTemplate {
    using SchematicType = std::conditional_t<Const, const Schematic, Schematic>;
    using ConnectionDataType
        = std::conditional_t<Const, const connection_t, connection_t>;

    /// This constructor is not regarded as a copy constructor,
    //   so we preserve trivially copyable
    friend InputTemplate<!Const>;
    friend ElementTemplate<Const>;
    explicit InputTemplate(SchematicType &schematic, element_id_t element_id,
                           connection_id_t input_index) noexcept;

   public:
    template <bool ConstOther>
    // NOLINTNEXTLINE(google-explicit-constructor)
    InputTemplate(InputTemplate<ConstOther> input) noexcept
        requires Const && (!ConstOther);

    template <bool ConstOther>
    auto operator==(InputTemplate<ConstOther> other) const noexcept -> bool;

    [[nodiscard]] operator connection_t() const noexcept;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_connection() const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> SchematicType &;
    [[nodiscard]] auto element_id() const noexcept -> element_id_t;
    [[nodiscard]] auto input_index() const noexcept -> connection_id_t;

    [[nodiscard]] auto element() const -> ElementTemplate<Const>;
    [[nodiscard]] auto has_connected_element() const -> bool;
    [[nodiscard]] auto connected_element_id() const -> element_id_t;
    [[nodiscard]] auto connected_output_index() const -> connection_id_t;

    [[nodiscard]] auto connected_element() const -> ElementTemplate<Const>;
    [[nodiscard]] auto connected_output() const -> OutputTemplate<Const>;

    void clear_connection() const
        requires(!Const);
    template <bool ConstOther>
    void connect(OutputTemplate<ConstOther> output) const
        requires(!Const);

    [[nodiscard]] auto is_inverted() const -> bool;
    void set_inverted(bool value) const
        requires(!Const);

   private:
    [[nodiscard]] auto connection_data_() const -> ConnectionDataType &;

    gsl::not_null<SchematicType *> schematic_;
    element_id_t element_id_;
    connection_id_t input_index_;
};

template <bool Const>
class Schematic::OutputTemplate {
   private:
    using SchematicType = std::conditional_t<Const, const Schematic, Schematic>;
    using ConnectionDataType
        = std::conditional_t<Const, const connection_t, connection_t>;

    friend OutputTemplate<!Const>;
    friend ElementTemplate<Const>;
    explicit OutputTemplate(SchematicType &schematic, element_id_t element_id,
                            connection_id_t output_index) noexcept;

   public:
    template <bool ConstOther>
    // NOLINTNEXTLINE(google-explicit-constructor)
    OutputTemplate(OutputTemplate<ConstOther> output) noexcept
        requires Const && (!ConstOther);

    template <bool ConstOther>
    auto operator==(OutputTemplate<ConstOther> other) const noexcept -> bool;

    [[nodiscard]] operator connection_t() const noexcept;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_connection() const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> SchematicType &;
    [[nodiscard]] auto element_id() const noexcept -> element_id_t;
    [[nodiscard]] auto output_index() const noexcept -> connection_id_t;

    [[nodiscard]] auto element() const -> ElementTemplate<Const>;
    [[nodiscard]] auto has_connected_element() const -> bool;
    [[nodiscard]] auto connected_element_id() const -> element_id_t;
    [[nodiscard]] auto connected_input_index() const -> connection_id_t;

    [[nodiscard]] auto connected_element() const -> ElementTemplate<Const>;
    [[nodiscard]] auto connected_input() const -> InputTemplate<Const>;

    void clear_connection() const
        requires(!Const);
    template <bool ConstOther>
    void connect(InputTemplate<ConstOther> input) const
        requires(!Const);

   private:
    [[nodiscard]] auto connection_data_() const -> ConnectionDataType &;

    gsl::not_null<SchematicType *> schematic_;
    element_id_t element_id_;
    connection_id_t output_index_;
};

}  // namespace logicsim

//
// Free functions
//

namespace logicsim {

auto validate_has_no_placeholders(const Schematic::ConstElement element) -> void;
auto validate_all_outputs_connected(const Schematic::ConstElement element) -> void;
auto validate_all_inputs_disconnected(const Schematic::ConstElement element) -> void;
auto validate_all_outputs_disconnected(const Schematic::ConstElement element) -> void;

class LineTree;
auto calculate_output_delays(const LineTree &line_tree) -> std::vector<delay_t>;

void add_output_placeholders(Schematic &schematic);

inline constexpr int BENCHMARK_DEFAULT_ELEMENTS {100};
inline constexpr double BENCHMARK_DEFAULT_CONNECTIVITY {0.75};

auto benchmark_schematic(int n_elements = BENCHMARK_DEFAULT_ELEMENTS) -> Schematic;

template <std::uniform_random_bit_generator G>
auto create_random_schematic(G &rng, int n_elements = BENCHMARK_DEFAULT_ELEMENTS,
                             double connection_ratio = BENCHMARK_DEFAULT_CONNECTIVITY)
    -> Schematic;

}  // namespace logicsim

#endif
