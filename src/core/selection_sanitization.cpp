#include "core/selection_sanitization.h"

#include "core/component/editable_circuit/modifier.h"
#include "core/geometry/offset.h"
#include "core/index/collision_index.h"
#include "core/layout.h"
#include "core/part_selection.h"
#include "core/selection.h"
#include "core/vocabulary/segment_part.h"

#include <folly/small_vector.h>
#include <gsl/gsl>

#include <exception>
#include <vector>

namespace logicsim {

template <>
auto format(SanitizeMode mode) -> std::string {
    switch (mode) {
        case SanitizeMode::shrink:
            return "shrink";
        case SanitizeMode::expand:
            return "expand";
    }
    std::terminate();
}

namespace {

class CrossingIndex {
   public:
    CrossingIndex(const CollisionIndex &index, ordered_line_t full_line);

    [[nodiscard]] auto is_colliding(point_t point) const -> bool;
    [[nodiscard]] auto is_colliding(offset_t offset) const -> bool;
    [[nodiscard]] auto max_offset() const -> offset_t;

   private:
    gsl::not_null<const CollisionIndex *> collision_index_;
    ordered_line_t full_line_;
};

CrossingIndex::CrossingIndex(const CollisionIndex &index, ordered_line_t full_line)
    : collision_index_ {&index}, full_line_ {full_line} {}

auto CrossingIndex::is_colliding(point_t point) const -> bool {
    return collision_index_->is_wires_crossing(point);
}

auto CrossingIndex::is_colliding(offset_t offset) const -> bool {
    const auto point = to_point(full_line_, offset);
    return is_colliding(point);
}

auto CrossingIndex::max_offset() const -> offset_t {
    return to_part(full_line_).end;
}

// free functions

auto is_colliding(part_t part, const CrossingIndex &index) -> bool {
    return index.is_colliding(part.begin) || index.is_colliding(part.end);
}

auto is_colliding(std::span<const part_t> parts, const CrossingIndex &index) -> bool {
    return std::ranges::any_of(
        parts, [&index](part_t part) { return is_colliding(part, index); });
}

auto find_lower(offset_t offset, const CrossingIndex &index, offset_t limit) -> offset_t {
    while (offset > limit) {
        if (!index.is_colliding(--offset)) {
            return offset;
        }
    }
    return offset;
}

auto find_higher(offset_t offset, const CrossingIndex &index,
                 offset_t limit) -> offset_t {
    while (offset < limit) {
        if (!index.is_colliding(++offset)) {
            return offset;
        }
    }
    return offset;
}

auto find_sanitized_part(const part_t part, const CrossingIndex &index,
                         const SanitizeMode mode) -> std::optional<part_t> {
    const auto new_offsets = [mode, &index, part]() {
        const auto begin_colliding = index.is_colliding(part.begin);
        const auto end_colliding = index.is_colliding(part.end);

        if (mode == SanitizeMode::expand) {
            return std::pair<offset_t, offset_t> {
                begin_colliding ? find_lower(part.begin, index, offset_t {0})
                                : part.begin,
                end_colliding ? find_higher(part.end, index, index.max_offset())
                              : part.end,
            };
        }
        return std::pair<offset_t, offset_t> {
            begin_colliding ? find_higher(part.begin, index, part.end) : part.begin,
            end_colliding ? find_lower(part.end, index, part.begin) : part.end,
        };
    }();

    if (new_offsets.first < new_offsets.second) {
        return part_t {new_offsets.first, new_offsets.second};
    }
    return std::nullopt;
}

auto find_sanitized_parts(std::span<const part_t> parts, const CrossingIndex &index,
                          const SanitizeMode mode) -> PartSelection {
    auto new_parts = part_selection::part_vector_t {};

    for (const auto &part : parts) {
        if (const auto new_part = find_sanitized_part(part, index, mode)) {
            new_parts.push_back(new_part.value());
        }
    }

    return PartSelection {std::move(new_parts)};
}

}  // namespace

auto is_sanitized(segment_part_t segment_part, const Layout &layout,
                  const CollisionIndex &index) -> bool {
    if (!is_inserted(segment_part.segment.wire_id)) {
        return true;
    }

    const auto line = get_line(layout, segment_part);
    return !index.is_wires_crossing(line.p0) && !index.is_wires_crossing(line.p1);
}

auto sanitize_part(segment_part_t segment_part, const Layout &layout,
                   const CollisionIndex &index, SanitizeMode mode) -> segment_part_t {
    if (!is_inserted(segment_part.segment.wire_id)) {
        return segment_part;
    }

    const auto full_line = get_line(layout, segment_part.segment);
    const auto crossing_index = CrossingIndex {index, full_line};

    if (const auto new_part =
            find_sanitized_part(segment_part.part, crossing_index, mode)) {
        return segment_part_t {segment_part.segment, new_part.value()};
    }
    return null_segment_part;
}

auto sanitize_part(segment_part_t segment_part,
                   const editable_circuit::Modifier &modifier,
                   SanitizeMode mode) -> segment_part_t {
    return sanitize_part(segment_part, modifier.circuit_data().layout,
                         modifier.circuit_data().index.collision_index(), mode);
}

namespace {

auto new_sanitize_parts(segment_t segment, const PartSelection &parts,
                        const Layout &layout, const CollisionIndex &index,
                        SanitizeMode &mode) -> std::optional<PartSelection> {
    if (!is_inserted(segment.wire_id)) {
        return std::nullopt;
    }

    const auto full_line = get_line(layout, segment);
    const auto crossing_index = CrossingIndex {index, full_line};

    if (is_colliding(parts, crossing_index)) {
        return find_sanitized_parts(parts, crossing_index, mode);
    }

    return std::nullopt;
}

}  // namespace

auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionIndex &index, SanitizeMode mode) -> void {
    std::vector<segment_t> to_delete {};

    for (const auto &entry : selection.selected_segments()) {
        const auto &[segment, parts] = entry;

        if (auto new_parts = new_sanitize_parts(segment, parts, layout, index, mode)) {
            if (new_parts->empty()) {
                to_delete.push_back(segment);
            } else {
                selection.set_selection(segment, std::move(*new_parts));
            }
        }
    }

    for (const auto &segment : to_delete) {
        selection.set_selection(segment, PartSelection {});
    }
}

}  // namespace logicsim
