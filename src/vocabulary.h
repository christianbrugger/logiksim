#ifndef LOGIKSIM_VOCABULARY_H
#define LOGIKSIM_VOCABULARY_H

#include "exceptions.h"
#include "format.h"

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
};

static_assert(std::is_trivial<circuit_id_t>::value);

struct element_id_t {
    using value_type = int32_t;
    value_type value;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const element_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const element_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };
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

// use struct packing ???
struct connection_t {
    element_id_t element_id;
    connection_id_t connection_id;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_trivial<connection_id_t>::value);

inline constexpr auto null_circuit = circuit_id_t {-1};
inline constexpr auto null_element = element_id_t {-1};
inline constexpr auto null_connection = connection_id_t {-1};

enum class orientation_t : uint8_t {
    right,
    left,
    up,
    down,
};

auto format(orientation_t state) -> std::string;

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

struct grid_t {
    using value_type = int16_t;
    value_type value;

    using difference_type = int32_t;
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

    [[nodiscard]] explicit constexpr operator double() const noexcept {
        return static_cast<double>(value);
    }

    [[nodiscard]] friend constexpr auto operator*(grid_t a, double b) -> double {
        return static_cast<double>(a) * b;
    }
};

static_assert(std::is_trivial<grid_t>::value);
static_assert(std::is_standard_layout<grid_t>::value);
static_assert(std::is_nothrow_default_constructible<grid_t>::value);

struct point_fine_t {
    double x;
    double y;

    [[nodiscard]] auto format() const -> std::string;
};

static_assert(std::is_trivial<point_fine_t>::value);

struct point_t {
    grid_t x;
    grid_t y;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] explicit constexpr operator point_fine_t() const noexcept {
        return point_fine_t {static_cast<double>(x), static_cast<double>(y)};
    }

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

constexpr auto is_orthogonal(point_t p0, point_t p1) noexcept -> bool {
    // xor disallows zero length lines
    return (p0.x == p1.x) ^ (p0.y == p1.y);
}

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

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const line_t &other) const -> bool = default;
};

static_assert(std::is_trivial_v<line_t>);
static_assert(std::is_trivially_constructible_v<line_t>);
static_assert(std::is_trivially_copyable<line_t>::value);
static_assert(std::is_trivially_copy_assignable<line_t>::value);

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

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::grid_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::grid_t &obj) const noexcept
        -> uint64_t {
        return detail::wyhash::hash(gsl::narrow_cast<uint64_t>(obj.value));
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::point_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::point_t &obj) const noexcept
        -> uint64_t {
        static_assert(std::has_unique_object_representations_v<logicsim::point_t>);
        return detail::wyhash::hash(&obj, sizeof(obj));
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

#endif