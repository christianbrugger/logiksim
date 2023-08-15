#ifndef LOGIKSIM_VOCABULARY_H
#define LOGIKSIM_VOCABULARY_H

#include "exception.h"
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
    unused,       // has no inputs and no logic
    placeholder,  // has no logic                 // TODO rename to ???
    wire,

    buffer_element,
    and_element,
    or_element,
    xor_element,

    button,
    clock_generator,
    flipflop_jk,
    shift_register,
    latch_d,
    flipflop_d,
    flipflop_ms_d,  // master slave

    sub_circuit,
};

template <>
auto format(ElementType type) -> std::string;

[[nodiscard]] auto is_logic_item(ElementType element_type) -> bool;

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

struct element_id_t {
    using value_type = int32_t;
    value_type value;

    explicit constexpr element_id_t() = default;
    explicit constexpr element_id_t(value_type value_) noexcept : value {value_} {};

    // explicit constexpr element_id_t(int value_)
    //     : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(unsigned int value_)
        : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(long value_)
        : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(unsigned long value_)
        : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(long long value_)
        : value {gsl::narrow<value_type>(value_)} {};
    explicit constexpr element_id_t(unsigned long long value_)
        : value {gsl::narrow<value_type>(value_)} {};

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

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return value >= 0;
    }

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

struct connection_t {
    element_id_t element_id;
    connection_id_t connection_id;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_t &other) const = default;
};

struct segment_index_t {
    using value_type = int32_t;
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

    auto operator--() noexcept -> segment_index_t & {
        --value;
        return *this;
    }

    auto operator--(int) noexcept -> segment_index_t {
        auto tmp = *this;
        operator--();
        return tmp;
    }
};

inline constexpr auto null_circuit = circuit_id_t {-1};
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

template <>
auto format(orientation_t state) -> std::string;

enum class display_state_t : uint8_t {
    normal,

    valid,
    colliding,

    temporary,
};

template <>
auto format(display_state_t state) -> std::string;

[[nodiscard]] auto is_inserted(display_state_t display_state) -> bool;

enum class InsertionMode {
    insert_or_discard,
    collisions,
    temporary,
};

template <>
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
using std::literals::chrono_literals::operator""ms;
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
    using value_type = std::chrono::duration<int32_t, std::nano>;
    value_type value {};

    [[nodiscard]] constexpr explicit delay_t() noexcept = default;

    [[nodiscard]] constexpr explicit delay_t(
        std::chrono::duration<int64_t, std::nano> delay)
        : value {delay} {
        if (value != delay) {
            throw_exception("delay cannot be represented.");
        }
    };

    [[nodiscard]] static constexpr auto epsilon() noexcept -> delay_t {
        return delay_t {++value_type::zero()};
    };

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const delay_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const delay_t &other) const = default;
};

struct time_rate_t {
    time_t rate_per_second;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const time_rate_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_rate_t &other) const = default;
};

//
// Simulation / Logic Types
//

using logic_small_vector_policy = folly::small_vector_policy::policy_size_type<uint32_t>;
using logic_small_vector_t = folly::small_vector<bool, 20, logic_small_vector_policy>;
static_assert(sizeof(logic_small_vector_t) == 24);

//
// Styling Types
//

}  // namespace logicsim

struct BLRgba32;

namespace logicsim {

struct color_t {
    using value_type = uint32_t;
    value_type value;

    color_t() = default;

    constexpr explicit color_t(uint32_t value_) noexcept : value {value_} {}

    constexpr explicit color_t(uint32_t r, uint32_t g, uint32_t b,
                               uint32_t a = 0xFFu) noexcept
        : value((r << 16) | (g << 8) | b | (a << 24)) {}

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const color_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const color_t &other) const = default;

    [[nodiscard]] constexpr auto r() const noexcept -> uint32_t {
        return (value >> 16) & 0xFFu;
    }

    [[nodiscard]] constexpr auto g() const noexcept -> uint32_t {
        return (value >> 8) & 0xFFu;
    }

    [[nodiscard]] constexpr auto b() const noexcept -> uint32_t {
        return (value >> 0) & 0xFFu;
    }

    [[nodiscard]] constexpr auto a() const noexcept -> uint32_t {
        return (value >> 24);
    }

    [[nodiscard]] operator BLRgba32() const noexcept;
};

namespace defaults {
constexpr static inline auto no_color = ::logicsim::color_t {0x00000000};

constexpr static inline auto color_black = ::logicsim::color_t {0xFF000000};
constexpr static inline auto color_white = ::logicsim::color_t {0xFFFFFFFF};
constexpr static inline auto color_light_gray = ::logicsim::color_t {0xFFD3D3D3};
constexpr static inline auto color_gray_90 = ::logicsim::color_t {0xFFE5E5E5};

constexpr static inline auto color_red = ::logicsim::color_t {0xFFFF0000};
constexpr static inline auto color_green = ::logicsim::color_t {0xFF009900};
constexpr static inline auto color_lime = ::logicsim::color_t {0xFF00FF00};
constexpr static inline auto color_yellow = ::logicsim::color_t {0xFFFFFF00};
constexpr static inline auto color_orange = ::logicsim::color_t {0xFFFF8000};
constexpr static inline auto color_dark_orange = ::logicsim::color_t {0xFFB36200};
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

    [[nodiscard]] constexpr explicit grid_t(long v) noexcept
        : value {gsl::narrow<value_type>(v)} {};
    [[nodiscard]] constexpr explicit grid_t(unsigned long v) noexcept
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

    [[nodiscard]] constexpr point_fine_t(grid_fine_t x_, grid_fine_t y_) noexcept
        : x {x_}, y {y_} {}

    [[nodiscard]] explicit constexpr point_fine_t(grid_t x_, grid_t y_) noexcept
        : x {grid_fine_t {x_}}, y {grid_fine_t {y_}} {}

    [[nodiscard]] explicit constexpr point_fine_t(point_t point) noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_fine_t &other) const
        -> bool = default;
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

    point_t() = default;

    constexpr point_t(grid_t x_, grid_t y_) noexcept : x {x_}, y {y_} {};

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

constexpr auto is_orthogonal(point_fine_t p0, point_fine_t p1) noexcept -> bool {
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

    [[nodiscard]] constexpr auto operator==(const ordered_line_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const ordered_line_t &other) const = default;
};

constexpr line_t::line_t(ordered_line_t line) : p0 {line.p0}, p1 {line.p1} {}

static_assert(std::is_trivial_v<ordered_line_t>);
static_assert(std::is_trivially_constructible_v<ordered_line_t>);
static_assert(std::is_trivially_copyable_v<ordered_line_t>);
static_assert(std::is_trivially_copy_assignable_v<ordered_line_t>);

struct line_fine_t {
    point_fine_t p0;
    point_fine_t p1;

    line_fine_t() = default;

    [[nodiscard]] explicit constexpr line_fine_t(point_fine_t p0_, point_fine_t p1_)
        : p0 {p0_}, p1 {p1_} {
        if (!is_orthogonal(p0_, p1_)) [[unlikely]] {
            throw_exception("line needs to be horizontal or vertical.");
        }
    };

    [[nodiscard]] constexpr line_fine_t(line_t line) noexcept
        : p0 {point_fine_t {line.p0}}, p1 {point_fine_t {line.p1}} {}

    [[nodiscard]] constexpr line_fine_t(ordered_line_t line) noexcept
        : p0 {point_fine_t {line.p0}}, p1 {point_fine_t {line.p1}} {}

    [[nodiscard]] explicit constexpr line_fine_t(point_t p0_, point_t p1_)
        : line_fine_t {point_fine_t {p0_}, point_fine_t {p1_}} {};

    [[nodiscard]] explicit constexpr line_fine_t(point_fine_t p0_, point_t p1_)
        : line_fine_t {p0_, point_fine_t {p1_}} {};

    [[nodiscard]] explicit constexpr line_fine_t(point_t p0_, point_fine_t p1_)
        : line_fine_t {point_fine_t {p0_}, p1_} {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const line_fine_t &other) const
        -> bool = default;
};

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
    }

    [[nodiscard]] explicit constexpr rect_fine_t(point_t p0_, point_t p1_)
        : rect_fine_t {point_fine_t {p0_}, point_fine_t {p1_}} {};

    [[nodiscard]] explicit constexpr rect_fine_t(rect_t rect) noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const rect_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const rect_fine_t &other) const = default;

    [[nodiscard]] constexpr auto operator+(point_fine_t other) const -> rect_fine_t {
        return rect_fine_t {p0 + other, p1 + other};
    }

    [[nodiscard]] constexpr auto operator-(point_fine_t other) const -> rect_fine_t {
        return rect_fine_t {p0 - other, p1 - other};
    }

    constexpr auto operator+=(point_fine_t other) -> rect_fine_t & {
        p0 += other;
        p1 += other;
        return *this;
    }

    constexpr auto operator-=(point_fine_t other) -> rect_fine_t & {
        p0 -= other;
        p1 -= other;
        return *this;
    }
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

    using difference_type = range_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type) &&
                  std::is_signed_v<difference_type>);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const offset_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const offset_t &other) const = default;

    auto operator++() -> offset_t & {
        return *this = *this + offset_t {1};
    }

    auto operator++(int) -> offset_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }

    auto operator--() -> offset_t & {
        return *this = *this - offset_t {1};
    }

    auto operator--(int) -> offset_t {
        auto tmp = *this;
        operator--();
        return tmp;
    }

    [[nodiscard]] constexpr auto operator+(offset_t other) const -> offset_t {
        auto result = value + other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<offset_t::value_type>(result)};
    }

    [[nodiscard]] constexpr auto operator-(offset_t other) const -> offset_t {
        auto result = value - other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<offset_t::value_type>(result)};
    }
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

struct part_copy_definition_t {
    part_t destination;
    part_t source;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const part_copy_definition_t &other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const part_copy_definition_t &other) const = default;
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

inline constexpr auto null_segment_part =
    segment_part_t {null_segment, part_t {offset_t {0}, offset_t {1}}};

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::grid_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::grid_t &obj) const noexcept
        -> uint64_t {
        return detail::wyhash::hash(static_cast<uint64_t>(obj.value));
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
struct ankerl::unordered_dense::hash<logicsim::element_id_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::element_id_t &obj) const noexcept
        -> uint64_t {
        return detail::wyhash::hash(static_cast<uint64_t>(obj.value));
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

#endif