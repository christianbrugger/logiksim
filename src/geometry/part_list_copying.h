#ifndef LOGICSIM_GEOMETRY_PART_LIST_COPYING_H
#define LOGICSIM_GEOMETRY_PART_LIST_COPYING_H

#include "geometry/part.h"
#include "vocabulary/offset.h"
#include "vocabulary/part.h"
#include "vocabulary/part_copy_definition.h"

#include <gsl/gsl>

#include <cassert>
#include <optional>
#include <stdexcept>
#include <vector>

namespace logicsim {

template <typename V = offset_t::difference_type>
[[nodiscard]] auto _get_shifted_part(part_t part, V shifted, V max_end)
    -> std::optional<part_t> {
    const auto begin = V {part.begin.value} + shifted;
    const auto end = std::min(V {part.end.value} + shifted, max_end);

    if (begin < end) {
        return part_t {offset_t {gsl::narrow_cast<offset_t::value_type>(begin)},
                       offset_t {gsl::narrow_cast<offset_t::value_type>(end)}};
    }
    return std::nullopt;
}

template <typename Container = std::vector<part_t>>
auto _add_intersecting_parts(const Container &source_entries,
                             Container &destination_entries, part_t part_destination)
    -> void {
    using V = offset_t::difference_type;

    auto shifted = V {part_destination.begin.value};
    auto max_end = V {part_destination.end.value};

    for (const part_t part : source_entries) {
        if (const auto new_part = _get_shifted_part<V>(part, shifted, max_end)) {
            assert(a_inside_b(*new_part, part_destination));
            destination_entries.push_back(*new_part);
        }
    }
}

template <typename Container = std::vector<part_t>>
[[nodiscard]] auto copy_parts(const Container &source_entries, part_t part_destination)
    -> Container {
    auto result = Container {};
    _add_intersecting_parts(source_entries, result, part_destination);
    return result;
}

template <typename Container = std::vector<part_t>>
auto copy_parts(const Container &source_entries, Container &destination_entries,
                part_t part_destination) -> void {
    bool original_empty = destination_entries.empty();

    _add_intersecting_parts(source_entries, destination_entries, part_destination);

    if (!original_empty) {
        sort_and_merge_parts(destination_entries);
    }
}

template <typename Container = std::vector<part_t>>
auto _add_intersecting_parts(const Container &source_entries,
                             Container &destination_entries, part_copy_definition_t parts)
    -> void {
    if (distance(parts.destination) != distance(parts.source)) {
        throw std::runtime_error("source and destination need to have the same size");
    }

    using V = offset_t::difference_type;

    auto shifted = V {parts.destination.begin.value} - V {parts.source.begin.value};
    auto max_end = V {parts.destination.end.value};

    for (const part_t part : source_entries) {
        if (const std::optional<part_t> res = intersect(part, parts.source)) {
            if (const auto new_part = _get_shifted_part<V>(*res, shifted, max_end)) {
                assert(a_inside_b(*new_part, parts.destination));
                destination_entries.push_back(*new_part);
            }
        }
    }
}

template <typename Container = std::vector<part_t>>
[[nodiscard]] auto copy_parts(const Container &source_entries,
                              part_copy_definition_t parts) -> Container {
    auto result = Container {};
    _add_intersecting_parts(source_entries, result, parts);
    return result;
}

template <typename Container = std::vector<part_t>>
auto copy_parts(const Container &source_entries, Container &destination_entries,
                part_copy_definition_t parts) -> void {
    bool original_empty = destination_entries.empty();

    _add_intersecting_parts(source_entries, destination_entries, parts);

    if (!original_empty) {
        sort_and_merge_parts(destination_entries);
    }
}

template <typename Container = std::vector<part_t>>
auto move_parts(Container &source_entries, Container &destination_entries,
                part_copy_definition_t parts) -> void {
    copy_parts(source_entries, destination_entries, parts);
    remove_part(source_entries, parts.source);
}

}  // namespace logicsim

#endif
