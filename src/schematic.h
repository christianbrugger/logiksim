#ifndef LOGIKSIM_SCHEMATIC_H
#define LOGIKSIM_SCHEMATIC_H

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

enum class ElementType : uint8_t {
    placeholder,  // has no logic
    wire,
    inverter_element,
    and_element,
    or_element,
    xor_element,

    clock_generator,
    flipflop_jk,
    shift_register,
};

auto format(ElementType type) -> std::string;

class Schematic {
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

    [[nodiscard]] auto element_count() const noexcept -> std::size_t;
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto is_element_id_valid(element_id_t element_id) const noexcept
        -> bool;
    [[nodiscard]] auto input_count() const noexcept -> std::size_t;
    [[nodiscard]] auto output_count() const noexcept -> std::size_t;

    [[nodiscard]] auto element(element_id_t element_id) -> Element;
    [[nodiscard]] auto element(element_id_t element_id) const -> ConstElement;
    [[nodiscard]] auto elements() noexcept -> ElementView;
    [[nodiscard]] auto elements() const noexcept -> ConstElementView;

    auto add_element(ElementType type, std::size_t input_count, std::size_t output_count)
        -> Element;
    auto clear() -> void;

    void validate(bool require_all_outputs_connected = false) const;

   private:
    static void validate_connection_data_(Schematic::ConnectionData connection_data);

    // TODO add packing
    struct ConnectionData {
        element_id_t element_id {null_element};
        connection_id_t index {null_connection};
    };

    // TODO add packing
    struct ElementData {
        // TODO use connection_id_t as counter
        folly::small_vector<ConnectionData, 3> input_data;
        folly::small_vector<ConnectionData, 3> output_data;

        ElementType type;
    };

    static_assert(sizeof(ConnectionData) == 8);
    static_assert(sizeof(ElementData) == 65);

    std::vector<ElementData> element_data_store_ {};
    std::size_t input_count_ {0};
    std::size_t output_count_ {0};
};

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
    using ElementDataType = std::conditional_t<Const, const ElementData, ElementData>;

    friend ElementTemplate<!Const>;
    friend Schematic;
    explicit ElementTemplate(SchematicType &schematic, element_id_t element_id) noexcept;

   public:
    /// This constructor is not regarded as a copy constructor,
    //   so we preserve trivially copyable
    template <bool ConstOther>
    // NOLINTNEXTLINE(google-explicit-constructor)
    ElementTemplate(ElementTemplate<ConstOther> element) noexcept
        requires Const && (!ConstOther);

    [[nodiscard]] operator element_id_t() const noexcept;

    template <bool ConstOther>
    auto operator==(ElementTemplate<ConstOther> other) const noexcept -> bool;

    [[nodiscard]] auto format(bool with_connections = false) const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> SchematicType *;
    [[nodiscard]] auto element_id() const noexcept -> element_id_t;

    [[nodiscard]] auto element_type() const -> ElementType;
    [[nodiscard]] auto input_count() const -> std::size_t;
    [[nodiscard]] auto output_count() const -> std::size_t;

    [[nodiscard]] auto input(connection_id_t input) const -> InputTemplate<Const>;
    [[nodiscard]] auto output(connection_id_t output) const -> OutputTemplate<Const>;

    [[nodiscard]] auto inputs() const -> InputViewTemplate<Const>;
    [[nodiscard]] auto outputs() const -> OutputViewTemplate<Const>;

   private:
    [[nodiscard]] auto element_data_() const -> ElementDataType &;

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
        = std::conditional_t<Const, const ConnectionData, ConnectionData>;

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

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_connection() const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> SchematicType *;
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
        = std::conditional_t<Const, const ConnectionData, ConnectionData>;

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

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto format_connection() const -> std::string;

    [[nodiscard]] auto schematic() const noexcept -> SchematicType *;
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
// Formatters
//

template <>
struct fmt::formatter<logicsim::Schematic> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::Schematic &obj, fmt::format_context &ctx)
        -> format_context::iterator {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::ElementType> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::ElementType &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

template <>
struct fmt::formatter<logicsim::Schematic::Element> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::Schematic::Element &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Schematic::ConstElement> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::Schematic::ConstElement &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Schematic::Input> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::Schematic::Input &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Schematic::ConstInput> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::Schematic::ConstInput &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Schematic::Output> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::Schematic::Output &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

template <>
struct fmt::formatter<logicsim::Schematic::ConstOutput> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::Schematic::ConstOutput &obj,
                       fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", obj.format());
    }
};

//
// Free functions
//

namespace logicsim {

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
