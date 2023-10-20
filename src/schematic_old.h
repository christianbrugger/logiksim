#ifndef LOGIKSIM_SCHEMATIC_OLD_H
#define LOGIKSIM_SCHEMATIC_OLD_H

#include "algorithm/range.h"
#include "format/struct.h"
#include "vocabulary.h"

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

class SchematicOld {
   public:
    // TODO move to schematic namespace
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

    explicit constexpr SchematicOld() = default;
    explicit SchematicOld(circuit_id_t circuit_id);
    explicit SchematicOld(delay_t wire_delay_per_distance);
    explicit SchematicOld(circuit_id_t circuit_id, delay_t wire_delay_per_distance);

    auto clear() -> void;
    auto swap(SchematicOld &other) noexcept -> void;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto circuit_id() const noexcept -> circuit_id_t;
    [[nodiscard]] auto element_count() const noexcept -> std::size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto is_element_id_valid(element_id_t element_id) const noexcept
        -> bool;
    [[nodiscard]] auto total_input_count() const noexcept -> std::size_t;
    [[nodiscard]] auto total_output_count() const noexcept -> std::size_t;

    [[nodiscard]] auto element_ids() const noexcept -> forward_range_t<element_id_t>;
    [[nodiscard]] auto element(element_id_t element_id) -> Element;
    [[nodiscard]] auto element(element_id_t element_id) const -> ConstElement;
    [[nodiscard]] auto elements() noexcept -> ElementView;
    [[nodiscard]] auto elements() const noexcept -> ConstElementView;

    [[nodiscard]] auto input(connection_t connection) -> Input;
    [[nodiscard]] auto input(connection_t connection) const -> ConstInput;
    [[nodiscard]] auto output(connection_t connection) -> Output;
    [[nodiscard]] auto output(connection_t connection) const -> ConstOutput;

    [[nodiscard]] auto wire_delay_per_distance() const -> delay_t;

    constexpr static inline auto no_history = delay_t {0ns};

    struct ElementData {
        ElementType element_type {ElementType::unused};
        connection_count_t input_count {0};
        connection_count_t output_count {0};

        circuit_id_t sub_circuit_id {null_circuit};
        logic_small_vector_t input_inverters {};
        std::vector<delay_t> output_delays {};
        delay_t history_length = no_history;
    };

    auto add_element(ElementData &&data) -> Element;
    // swaps the element with last one and deletes it
    auto swap_and_delete_element(element_id_t element_id) -> element_id_t;
    auto swap_elements(element_id_t element_id_0, element_id_t element_id_1) -> void;

   private:
    auto swap_element_data(element_id_t element_id_1, element_id_t element_id_2,
                           bool update_connections) -> void;
    auto update_swapped_connections(element_id_t new_element_id,
                                    element_id_t old_element_id) -> void;
    auto delete_last_element(bool clear_connections) -> void;

    // output_delays type
    using policy = folly::small_vector_policy::policy_size_type<uint64_t>;
    using output_delays_t = folly::small_vector<delay_t, 3, policy>;
    static_assert(sizeof(output_delays_t) == 32);

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

    std::size_t total_input_count_ {0};
    std::size_t total_output_count_ {0};
    circuit_id_t circuit_id_ {0};

    delay_t wire_delay_per_distance_ {0ns};
};

auto swap(SchematicOld &a, SchematicOld &b) noexcept -> void;

}  // namespace logicsim

template <>
auto std::swap(logicsim::SchematicOld &a, logicsim::SchematicOld &b) noexcept -> void;

namespace logicsim {

template <bool IsInput, typename T>
auto to_connection(T &&schematic, connection_t connection_data)
    requires std::same_as<std::remove_cvref_t<T>, SchematicOld>
{
    if constexpr (IsInput) {
        return schematic.input(connection_data);
    } else {
        return schematic.output(connection_data);
    }
}

template <class T>
concept ElementOrConnection = std::convertible_to<T, SchematicOld::ConstElement> ||
                              std::convertible_to<T, SchematicOld::ConstInput> ||
                              std::convertible_to<T, SchematicOld::ConstOutput>;

template <bool Const>
class SchematicOld::ElementIteratorTemplate {
   public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    using value_type = SchematicOld::ElementTemplate<Const>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    using schematic_type = std::conditional_t<Const, const SchematicOld, SchematicOld>;

    // needs to be default constructable, so ElementView can become a range and view
    ElementIteratorTemplate() = default;
    [[nodiscard]] explicit ElementIteratorTemplate(schematic_type &schematic,
                                                   element_id_t element_id) noexcept;

    [[nodiscard]] auto operator*() const -> value_type;
    // Prefix increment
    auto operator++() noexcept -> SchematicOld::ElementIteratorTemplate<Const> &;
    // Postfix increment
    auto operator++(int) noexcept -> SchematicOld::ElementIteratorTemplate<Const>;

    [[nodiscard]] auto operator==(const ElementIteratorTemplate &right) const noexcept
        -> bool;
    [[nodiscard]] auto operator-(
        const SchematicOld::ElementIteratorTemplate<Const> &right) const
        -> difference_type;

   private:
    schematic_type *schematic_ {};  // can be null, because default constructable
    element_id_t element_id_ {};
};

template <bool Const>
class SchematicOld::ElementViewTemplate {
   public:
    using iterator_type = SchematicOld::ElementIteratorTemplate<Const>;
    using schematic_type = std::conditional_t<Const, const SchematicOld, SchematicOld>;

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
    std::ranges::enable_view<logicsim::SchematicOld::ElementViewTemplate<Const>> = true;

template <bool Const>
inline constexpr bool std::ranges::enable_borrowed_range<
    logicsim::SchematicOld::ElementViewTemplate<Const>> = true;

namespace logicsim {

template <bool Const>
class SchematicOld::ElementTemplate {
    using SchematicType = std::conditional_t<Const, const SchematicOld, SchematicOld>;

    friend ElementTemplate<!Const>;
    friend SchematicOld;
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

    [[nodiscard]] auto input_count() const -> connection_count_t;
    [[nodiscard]] auto output_count() const -> connection_count_t;

    [[nodiscard]] auto input(connection_id_t input) const -> InputTemplate<Const>;
    [[nodiscard]] auto output(connection_id_t output) const -> OutputTemplate<Const>;

    [[nodiscard]] auto inputs() const -> InputViewTemplate<Const>;
    [[nodiscard]] auto outputs() const -> OutputViewTemplate<Const>;

    auto clear_all_connection() const -> void
        requires(!Const);

    auto set_history_length(delay_t value) const -> void
        requires(!Const);
    auto set_output_delays(std::vector<delay_t> delays) const -> void
        requires(!Const);

   private:
    gsl::not_null<SchematicType *> schematic_;
    element_id_t element_id_;
};

template <bool Const, bool IsInput>
class SchematicOld::ConnectionIteratorTemplate {
   public:
    std::optional<SchematicOld::ElementTemplate<Const>> element {};
    connection_id_t connection_id {};

    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;

    using value_type = std::conditional_t<IsInput, SchematicOld::InputTemplate<Const>,
                                          SchematicOld::OutputTemplate<Const>>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

    [[nodiscard]] auto operator*() const -> value_type;
    // Prefix increment
    auto operator++() noexcept
        -> SchematicOld::ConnectionIteratorTemplate<Const, IsInput> &;
    // Postfix increment
    auto operator++(int) noexcept -> ConnectionIteratorTemplate;

    [[nodiscard]] auto operator==(const ConnectionIteratorTemplate &right) const noexcept
        -> bool;
    [[nodiscard]] auto operator-(const ConnectionIteratorTemplate &right) const
        -> difference_type;
};

template <bool Const, bool IsInput>
class SchematicOld::ConnectionViewTemplate {
   public:
    using iterator_type = SchematicOld::ConnectionIteratorTemplate<Const, IsInput>;

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
inline constexpr bool std::ranges::enable_view<
    logicsim::SchematicOld::ConnectionViewTemplate<Const, IsInput>> = true;

template <bool Const, bool IsInput>
inline constexpr bool std::ranges::enable_borrowed_range<
    logicsim::SchematicOld::ConnectionViewTemplate<Const, IsInput>> = true;

namespace logicsim {

template <bool Const>
class SchematicOld::InputTemplate {
    using SchematicType = std::conditional_t<Const, const SchematicOld, SchematicOld>;
    using ConnectionDataType =
        std::conditional_t<Const, const connection_t, connection_t>;

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
class SchematicOld::OutputTemplate {
   private:
    using SchematicType = std::conditional_t<Const, const SchematicOld, SchematicOld>;
    using ConnectionDataType =
        std::conditional_t<Const, const connection_t, connection_t>;

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

    [[nodiscard]] auto delay() const -> const delay_t;
    auto set_delay(delay_t value) const -> void
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

void add_output_placeholders(SchematicOld &schematic);

}  // namespace logicsim

#endif
