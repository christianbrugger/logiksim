#ifndef LOGICSIM_VOCABULARY_CONNECTION_H
#define LOGICSIM_VOCABULARY_CONNECTION_H

#include "format/struct.h"
#include "vocabulary/connection_id.h"
#include "vocabulary/wire_id.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>

#include <compare>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifies an input or output of a specific circuit element.
 *
 * Class invariants:
 *     * element_id and connection_id are either both valid or null.
 */
struct connection_t {
    element_id_t element_id {null_element};
    connection_id_t connection_id {null_connection_id};

    [[nodiscard]] explicit constexpr connection_t() noexcept = default;
    [[nodiscard]] explicit constexpr connection_t(element_id_t element_id_,
                                                  connection_id_t connection_id_);

    /**
     * @brief: The bool cast tests if this segment is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<connection_t>);
static_assert(std::is_trivially_copy_assignable_v<connection_t>);

struct input_t : public connection_t {
    using connection_t::connection_t;
};

static_assert(std::is_trivially_copyable_v<input_t>);
static_assert(std::is_trivially_copy_assignable_v<input_t>);

struct output_t : public connection_t {
    using connection_t::connection_t;
};

static_assert(std::is_trivially_copyable_v<output_t>);
static_assert(std::is_trivially_copy_assignable_v<output_t>);

constexpr inline auto null_connection = connection_t {};
constexpr inline auto null_input = input_t {};
constexpr inline auto null_output = output_t {};

//
// Implemention
//

constexpr connection_t::connection_t(element_id_t element_id_,
                                     connection_id_t connection_id_)
    : element_id {element_id_}, connection_id {connection_id_} {
    if (bool {element_id} != bool {connection_id}) [[unlikely]] {
        throw std::runtime_error("Connection cannot be partially null.");
    }
}

constexpr connection_t::operator bool() const noexcept {
    return bool {element_id};
}

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::input_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::input_t &obj) const noexcept
        -> uint64_t {
        static_assert(std::is_same_v<int32_t, logicsim::element_id_t::value_type>);
        static_assert(std::is_same_v<int16_t, logicsim::connection_id_t::value_type>);

        return logicsim::wyhash_64_bit(obj.element_id.value,
                                       int32_t {obj.connection_id.value});
    }
};

template <>
struct ankerl::unordered_dense::hash<logicsim::output_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::output_t &obj) const noexcept
        -> uint64_t {
        static_assert(std::is_same_v<int32_t, logicsim::element_id_t::value_type>);
        static_assert(std::is_same_v<int16_t, logicsim::connection_id_t::value_type>);

        return logicsim::wyhash_64_bit(obj.element_id.value,
                                       int32_t {obj.connection_id.value});
    }
};

#endif
