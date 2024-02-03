#ifndef LOGICSIM_PART_SELECTION_H
#define LOGICSIM_PART_SELECTION_H

#include "format/struct.h"
#include "vocabulary/part.h"
#include "vocabulary/part_copy_definition.h"

#include <folly/small_vector.h>

#include <compare>

namespace logicsim {

struct part_copy_definition_t;

namespace part_selection {

// all parts are part of a line, so at maximum #grids / 2 selections are possible.
using part_vector_size_type = uint16_t;
constexpr inline auto part_vector_size = 2;

using part_vector_t = folly::small_vector<
    part_t, part_vector_size,
    folly::small_vector_policy::policy_size_type<part_vector_size_type>>;

static_assert(sizeof(part_vector_t) == 10);

}  // namespace part_selection

/**
 * @brief: Selected parts on a line.
 *
 * Class-invariants:
 *     + parts_ are sorted ascending.
 *     + adjacent parts do not touch (they are merged).
 */
class PartSelection {
   public:
    using part_vector_t = part_selection::part_vector_t;
    using iterator = part_vector_t::const_iterator;
    using value_type = part_t;

   public:
    [[nodiscard]] explicit constexpr PartSelection() = default;
    [[nodiscard]] explicit PartSelection(part_t part);
    [[nodiscard]] explicit PartSelection(part_vector_t&& parts);
    [[nodiscard]] auto inverted_selection(part_t part) const -> PartSelection;

    [[nodiscard]] auto format() const -> std ::string;
    [[nodiscard]] auto operator==(const PartSelection&) const -> bool = default;
    [[nodiscard]] auto operator<=>(const PartSelection&) const = default;

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto allocated_size() const -> std::size_t;

    auto add_part(part_t part) -> void;
    auto remove_part(part_t part) -> void;
    auto copy_parts(const PartSelection& source, part_copy_definition_t copy_definition)
        -> void;

    [[nodiscard]] auto begin() const -> iterator;
    [[nodiscard]] auto end() const -> iterator;
    [[nodiscard]] auto front() const -> part_t;
    [[nodiscard]] auto back() const -> part_t;
    [[nodiscard]] auto max_offset() const -> offset_t;

   private:
    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    part_vector_t parts_ {};
};

static_assert(sizeof(PartSelection) == 10);

[[nodiscard]] auto copy_parts(const PartSelection& source,
                              part_copy_definition_t copy_definition) -> PartSelection;

struct move_definition_t {
    PartSelection& destination;
    PartSelection& source;
    part_copy_definition_t copy_definition;
};

/**
 * @brief: Moves parts between two different part selections.
 */
auto move_parts(move_definition_t attrs) -> void;

/**
 * @brief: Moves parts within the same part selection.
 */
auto move_parts(PartSelection& parts, part_copy_definition_t copy_definition) -> void;

}  // namespace logicsim

#endif
