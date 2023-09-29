#ifndef LOGICSIM_VOCABULARY_PART_COPY_DEFINITION_H
#define LOGICSIM_VOCABULARY_PART_COPY_DEFINITION_H

#include "format/struct.h"
#include "vocabulary/part.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Defines a copy operation from part source to part destination.
 */
struct part_copy_definition_t {
    part_t destination;
    part_t source;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const part_copy_definition_t &other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const part_copy_definition_t &other) const = default;
};

static_assert(std::is_aggregate_v<part_copy_definition_t>);
static_assert(std::is_trivially_copyable_v<part_copy_definition_t>);
static_assert(std::is_trivially_copy_assignable_v<part_copy_definition_t>);

}  // namespace logicsim

#endif
