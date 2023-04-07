#ifndef LOGIKSIM_VOCABULARY_H
#define LOGIKSIM_VOCABULARY_H

#include "exceptions.h"
#include "format.h"
#include "hashing.h"
#include "range.h"

#include <ankerl/unordered_dense.h>
#include <fmt/core.h>
#include <gsl/gsl>

#include <chrono>
#include <compare>
#include <cstdint>
#include <string>
#include <type_traits>

namespace logicsim {

//
// Schematic Types
//

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

    sub_circuit,
};

auto format(ElementType type) -> std::string;

struct circuit_id_t {
    using value_type = int16_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const circuit_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const circuit_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return value >= 0;
    }
};

static_assert(std::is_trivial<circuit_id_t>::value);

// global unchanging identifier of an selection
// increments for each selection added
struct selection_key_t {
    using value_type = int64_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const selection_key_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const selection_key_t &other) const = default;

    auto operator++() noexcept -> selection_key_t &;
    auto operator++(int) noexcept -> selection_key_t;
};

static_assert(std::is_trivial<selection_key_t>::value);

struct element_id_t {
    using value_type = int32_t;
    value_type value;

    using difference_type = range_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const element_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const element_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return value >= 0;
    }

    auto operator++() noexcept -> element_id_t & {
        ++value;
        return *this;
    }

    auto operator++(int) noexcept -> element_id_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }
};

static_assert(std::is_trivial<element_id_t>::value);

struct connection_id_t {
    using value_type = int8_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    auto operator++() noexcept -> connection_id_t & {
        ++value;
        return *this;
    }

    auto operator++(int) noexcept -> connection_id_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }
};

static_assert(std::is_trivial<connection_id_t>::value);

// TODO use struct packing?
struct connection_t {
    element_id_t element_id;
    connection_id_t connection_id;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_t &other) const = default;
};

struct segment_index_t {
    using value_type = int16_t;
    value_type value;

    using difference_type = range_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_index_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_index_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return value >= 0;
    }

    auto operator++() noexcept -> segment_index_t & {
        ++value;
        return *this;
    }

    auto operator++(int) noexcept -> segment_index_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }
};

inline constexpr auto null_circuit = circuit_id_t {-1};
inline constexpr auto null_selection_key = selection_key_t {-1};
inline constexpr auto null_element = element_id_t {-1};
inline constexpr auto null_connection = connection_id_t {-1};

inline constexpr auto null_segment_index = segment_index_t {-1};

// TODO use struct packing?
struct segment_t {
    element_id_t element_id {null_element};
    segment_index_t segment_index {null_segment_index};

    constexpr segment_t() noexcept = default;

    constexpr segment_t(element_id_t element_id_, segment_index_t segment_index_)
        : element_id {element_id_}, segment_index {segment_index_} {
        if ((element_id == null_element) ^ (segment_index == null_segment_index))
            [[unlikely]] {
            throw_exception("Segment cannot be partially null.");
        }
    }

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_t &other) const = default;

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return element_id.value >= 0;
    }
};

inline constexpr auto null_segment = segment_t {};

//
// Display Type
//
enum class orientation_t : uint8_t {
    right,
    left,
    up,
    down,

    undirected,
};

auto format(orientation_t state) -> std::string;

enum class display_state_t : uint8_t {
    normal,

    new_valid,
    new_colliding,

    new_temporary,
};

auto format(display_state_t state) -> std::string;

[[nodiscard]] auto is_inserted(display_state_t display_state) -> bool;

enum class InsertionMode {
    insert_or_discard,
    collisions,
    temporary,
};

auto format(InsertionMode mode) -> std::string;

[[nodiscard]] auto to_insertion_mode(display_state_t display_state) -> InsertionMode;

//
// Time Types
//

#ifdef _MSC_VER
#pragma warning(push)
// false-positive: literal suffix identifiers are reserved
#pragma warning(disable : 4455)
#endif
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""ns;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

struct time_t {
    using value_type = std::chrono::duration<int64_t, std::nano>;
    using rep = value_type::rep;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const time_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_t &other) const = default;

    [[nodiscard]] static constexpr auto zero() noexcept -> time_t {
        return time_t {value_type::zero()};
    };

    [[nodiscard]] static constexpr auto epsilon() noexcept -> time_t {
        return time_t {++value_type::zero()};
    };

    [[nodiscard]] static constexpr auto min() noexcept -> time_t {
        return time_t {value_type::min()};
    };

    [[nodiscard]] static constexpr auto max() noexcept -> time_t {
        return time_t {value_type::max()};
    };
};

static_assert(std::is_trivial<time_t>::value);

struct delay_t {
    std::chrono::duration<int32_t, std::nano> value {};

    [[nodiscard]] constexpr explicit delay_t() noexcept = default;

    [[nodiscard]] constexpr explicit delay_t(
        std::chrono::duration<int64_t, std::nano> delay)
        : value {delay} {
        if (value != delay) {
            throw_exception("delay cannot be represented.");
        }
    };

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const delay_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const delay_t &other) const = default;
};

//
// Styling Types
//

struct color_t {
    using value_type = uint32_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const color_t &other) const -> bool = default;

    [[nodiscard]] constexpr auto r() const noexcept -> int {
        return (value >> 16) & 0xFFu;
    }

    [[nodiscard]] constexpr auto g() const noexcept -> int {
        return (value >> 8) & 0xFFu;
    }

    [[nodiscard]] constexpr auto b() const noexcept -> int {
        return (value >> 0) & 0xFFu;
    }

    [[nodiscard]] constexpr auto a() const noexcept -> int {
        return (value >> 24);
    }
};

namespace defaults {
constexpr static inline auto color_black = ::logicsim::color_t {0xFF000000};
constexpr static inline auto color_red = ::logicsim::color_t {0xFFFF0000};
constexpr static inline auto color_green = ::logicsim::color_t {0xFF009900};
constexpr static inline auto color_lime = ::logicsim::color_t {0xFF00FF00};
constexpr static inline auto color_yellow = ::logicsim::color_t {0xFFFFFF00};
constexpr static inline auto color_orange = ::logicsim::color_t {0xFFFF8000};
constexpr static inline auto color_blue = ::logicsim::color_t {0xFF0000FF};
}  // namespace defaults

//
// Spacial Types
//

// Are float enough for our fine grid?
// The highest representable float integer is 2**24 = 16'777'216.
// At 2**15 = 32'768 we have 9 fractional bits, a resolution of 2**-9 = 0.001953125.

using grid_fine_t = double;

struct grid_t {
    using value_type = int16_t;
    value_type value;

    using difference_type = range_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    grid_t() = default;

    [[nodiscard]] constexpr grid_t(value_type v) noexcept : value {v} {};
    [[nodiscard]] constexpr explicit grid_t(int v) noexcept
        : value {gsl::narrow<value_type>(v)} {};
    [[nodiscard]] constexpr explicit grid_t(unsigned int v) noexcept
        : value {gsl::narrow<value_type>(v)} {};
    [[nodiscard]] constexpr explicit grid_t(long long v) noexcept
        : value {gsl::narrow<value_type>(v)} {};
    [[nodiscard]] constexpr explicit grid_t(unsigned long long v) noexcept
        : value {gsl::narrow<value_type>(v)} {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const grid_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const grid_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept {
        return std::numeric_limits<value_type>::min();
    };

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    // arithmetic (overflow is checked)

    [[nodiscard]] constexpr auto operator+(grid_t other) const -> grid_t {
        auto result = value + other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<grid_t::value_type>(result)};
    }

    [[nodiscard]] constexpr auto operator-(grid_t other) const -> grid_t {
        auto result = value - other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<grid_t::value_type>(result)};
    }

    auto operator++() -> grid_t & {
        return *this = *this + grid_t {1};
    }

    auto operator++(int) -> grid_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }

    // conversions

    [[nodiscard]] explicit constexpr operator int() const noexcept {
        return static_cast<int>(value);
    }

    [[nodiscard]] explicit constexpr operator grid_fine_t() const noexcept {
        return static_cast<grid_fine_t>(value);
    }

    [[nodiscard]] friend constexpr auto operator*(grid_t a, grid_fine_t b)
        -> grid_fine_t {
        return static_cast<grid_fine_t>(a) * b;
    }
};

static_assert(std::is_trivial<grid_t>::value);
static_assert(std::is_standard_layout<grid_t>::value);
static_assert(std::is_nothrow_default_constructible<grid_t>::value);

struct point_t;

struct point_fine_t {
    grid_fine_t x;
    grid_fine_t y;

    point_fine_t() = default;

    [[nodiscard]] explicit constexpr point_fine_t(grid_fine_t x_, grid_fine_t y_) noexcept
        : x {x_}, y {y_} {}

    [[nodiscard]] explicit constexpr point_fine_t(grid_t x_, grid_t y_) noexcept
        : x {grid_fine_t {x_}}, y {grid_fine_t {y_}} {}

    [[nodiscard]] explicit constexpr point_fine_t(point_t point) noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_fine_t &other) const -> bool
        = default;
    [[nodiscard]] constexpr auto operator<=>(const point_fine_t &other) const = default;

    [[nodiscard]] constexpr auto operator+(point_fine_t other) const -> point_fine_t {
        return point_fine_t {x + other.x, y + other.y};
    }

    [[nodiscard]] constexpr auto operator-(point_fine_t other) const -> point_fine_t {
        return point_fine_t {x - other.x, y - other.y};
    }

    constexpr auto operator+=(point_fine_t other) -> point_fine_t & {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr auto operator-=(point_fine_t other) -> point_fine_t & {
        x -= other.x;
        y -= other.y;
        return *this;
    }
};

static_assert(std::is_trivial<point_fine_t>::value);

struct point_t {
    grid_t x;
    grid_t y;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_t &other) const -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const point_t &other) const = default;

    [[nodiscard]] constexpr auto operator+(point_t other) const -> point_t {
        return {x + other.x, y + other.y};
    }

    [[nodiscard]] constexpr auto operator-(point_t other) const -> point_t {
        return {x - other.x, y - other.y};
    }
};

static_assert(std::is_trivial<point_t>::value);

constexpr point_fine_t::point_fine_t(point_t point) noexcept
    : x {grid_fine_t {point.x}}, y {grid_fine_t {point.y}} {}

constexpr auto is_orthogonal(point_t p0, point_t p1) noexcept -> bool {
    // xor disallows zero length lines
    return (p0.x == p1.x) ^ (p0.y == p1.y);
}
struct ordered_line_t;

// a line is either horizontal or vertical
struct line_t {
    point_t p0;
    point_t p1;

    line_t() = default;

    [[nodiscard]] explicit constexpr line_t(point_t p0_, point_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (!is_orthogonal(p0_, p1_)) [[unlikely]] {
            throw_exception("line needs to be horizontal or vertical.");
        }
    };

    [[nodiscard]] explicit constexpr line_t(ordered_line_t line);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const line_t &other) const -> bool = default;
};

static_assert(std::is_trivial_v<line_t>);
static_assert(std::is_trivially_constructible_v<line_t>);
static_assert(std::is_trivially_copyable_v<line_t>);
static_assert(std::is_trivially_copy_assignable_v<line_t>);

struct ordered_line_t {
    point_t p0;
    point_t p1;

    ordered_line_t() = default;

    [[nodiscard]] explicit constexpr ordered_line_t(point_t p0_, point_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (!(is_orthogonal(p0_, p1_) && p0_ < p1_)) [[unlikely]] {
            throw_exception(
                "line needs to be horizontal or vertical and points need to be ordered.");
        }
    }

    [[nodiscard]] explicit constexpr ordered_line_t(line_t line) noexcept
        : p0 {line.p0 < line.p1 ? line.p0 : line.p1},
          p1 {line.p0 < line.p1 ? line.p1 : line.p0} {}

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const ordered_line_t &other) const -> bool
        = default;
    [[nodiscard]] constexpr auto operator<=>(const ordered_line_t &other) const = default;
};

constexpr line_t::line_t(ordered_line_t line) : p0 {line.p0}, p1 {line.p1} {}

static_assert(std::is_trivial_v<ordered_line_t>);
static_assert(std::is_trivially_constructible_v<ordered_line_t>);
static_assert(std::is_trivially_copyable_v<ordered_line_t>);
static_assert(std::is_trivially_copy_assignable_v<ordered_line_t>);

struct rect_t;

struct rect_fine_t {
    point_fine_t p0;
    point_fine_t p1;

    rect_fine_t() = default;

    [[nodiscard]] explicit constexpr rect_fine_t(point_fine_t p0_, point_fine_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (p0_.x > p1_.x || p0_.y > p1_.y) [[unlikely]] {
            throw_exception("point in rect_t need to be ordered");
        }
    };

    [[nodiscard]] explicit constexpr rect_fine_t(point_t p0_, point_t p1_)
        : rect_fine_t {point_fine_t {p0_}, point_fine_t {p1_}} {};

    [[nodiscard]] explicit constexpr rect_fine_t(rect_t rect) noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const rect_fine_t &other) const -> bool
        = default;
};

struct rect_t {
    point_t p0;
    point_t p1;

    rect_t() = default;

    [[nodiscard]] explicit constexpr rect_t(point_t p0_, point_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (p0_.x > p1_.x || p0_.y > p1_.y) [[unlikely]] {
            throw_exception("point in rect_t need to be ordered");
        }
    };

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const rect_t &other) const -> bool = default;
};

constexpr rect_fine_t::rect_fine_t(rect_t rect) noexcept
    : p0 {point_fine_t {rect.p0}}, p1 {point_fine_t {rect.p1}} {}

struct offset_t {
    using value_type = std::make_unsigned_t<grid_t::value_type>;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const offset_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const offset_t &other) const = default;
};

static_assert(sizeof(offset_t::value_type) == sizeof(grid_t::value_type));
static_assert(std::is_trivial<offset_t>::value);
static_assert(std::is_standard_layout<offset_t>::value);
static_assert(std::is_nothrow_default_constructible<offset_t>::value);

struct part_t {
    offset_t begin;
    offset_t end;

    [[nodiscard]] explicit constexpr part_t(offset_t begin_, offset_t end_)
        : begin {begin_}, end {end_} {
        if (!(begin_ < end_)) [[unlikely]] {
            throw_exception("begin needs to be smaller than end.");
        }
    };

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const part_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const part_t &other) const = default;
};

struct segment_part_t {
    segment_t segment;
    part_t part;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_part_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_part_t &other) const = default;

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return bool {segment};
    }
};

inline constexpr auto null_segment_part
    = segment_part_t {null_segment, part_t {offset_t {0}, offset_t {1}}};

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::grid_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::grid_t &obj) const noexcept
        -> uint64_t {
        return detail::wyhash::hash(obj.value);
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::point_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::point_t &obj) const noexcept
        -> uint64_t {
        using value_type = logicsim::point_t;
        using bit_type = uint32_t;

        static_assert(std::has_unique_object_representations_v<value_type>);
        static_assert(sizeof(value_type) == sizeof(bit_type));

        return detail::wyhash::hash(std::bit_cast<bit_type>(obj));
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::selection_key_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::selection_key_t &obj) const noexcept
        -> uint64_t {
        return detail::wyhash::hash(obj.value);
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::element_id_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::element_id_t &obj) const noexcept
        -> uint64_t {
        return detail::wyhash::hash(obj.value);
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::segment_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::segment_t &obj) const noexcept
        -> uint64_t {
        return logicsim::hash_8_byte(static_cast<uint32_t>(obj.element_id.value),
                                     static_cast<uint32_t>(obj.segment_index.value));
    }
};

//
// Formatters
//

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
struct fmt::formatter<logicsim::InsertionMode> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::InsertionMode &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

template <>
struct fmt::formatter<logicsim::display_state_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::display_state_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

template <>
struct fmt::formatter<logicsim::orientation_t> {
    static constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    static auto format(const logicsim::orientation_t &obj, fmt::format_context &ctx) {
        return fmt::format_to(ctx.out(), "{}", ::logicsim::format(obj));
    }
};

#endif