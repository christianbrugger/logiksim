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

class CrossingCache {
   public:
    CrossingCache(const CollisionIndex &cache, ordered_line_t full_line);

    [[nodiscard]] auto is_colliding(point_t point) const -> bool;
    [[nodiscard]] auto is_colliding(offset_t offset) const -> bool;
    [[nodiscard]] auto max_offset() const -> offset_t;

   private:
    gsl::not_null<const CollisionIndex *> collision_index_;
    ordered_line_t full_line_;
};

CrossingCache::CrossingCache(const CollisionIndex &cache, ordered_line_t full_line)
    : collision_index_ {&cache}, full_line_ {full_line} {}

auto CrossingCache::is_colliding(point_t point) const -> bool {
    return collision_index_->is_wires_crossing(point);
}

auto CrossingCache::is_colliding(offset_t offset) const -> bool {
    const auto point = to_point(full_line_, offset);
    return is_colliding(point);
}

auto CrossingCache::max_offset() const -> offset_t {
    return to_part(full_line_).end;
}

// free functions

auto is_colliding(part_t part, const CrossingCache &cache) -> bool {
    return cache.is_colliding(part.begin) || cache.is_colliding(part.end);
}

auto is_colliding(std::span<const part_t> parts, const CrossingCache &cache) -> bool {
    return std::ranges::any_of(
        parts, [&cache](part_t part) { return is_colliding(part, cache); });
}

auto find_lower(offset_t offset, const CrossingCache &cache, offset_t limit) -> offset_t {
    while (offset > limit) {
        if (!cache.is_colliding(--offset)) {
            return offset;
        }
    }
    return offset;
}

auto find_higher(offset_t offset, const CrossingCache &cache,
                 offset_t limit) -> offset_t {
    while (offset < limit) {
        if (!cache.is_colliding(++offset)) {
            return offset;
        }
    }
    return offset;
}

auto find_sanitized_part(const part_t part, const CrossingCache &cache,
                         const SanitizeMode mode) -> std::optional<part_t> {
    const auto new_offsets = [mode, &cache, part]() {
        const auto begin_colliding = cache.is_colliding(part.begin);
        const auto end_colliding = cache.is_colliding(part.end);

        if (mode == SanitizeMode::expand) {
            return std::pair<offset_t, offset_t> {
                begin_colliding ? find_lower(part.begin, cache, offset_t {0})
                                : part.begin,
                end_colliding ? find_higher(part.end, cache, cache.max_offset())
                              : part.end,
            };
        }
        return std::pair<offset_t, offset_t> {
            begin_colliding ? find_higher(part.begin, cache, part.end) : part.begin,
            end_colliding ? find_lower(part.end, cache, part.begin) : part.end,
        };
    }();

    if (new_offsets.first < new_offsets.second) {
        return part_t {new_offsets.first, new_offsets.second};
    }
    return std::nullopt;
}

auto find_sanitized_parts(std::span<const part_t> parts, const CrossingCache &cache,
                          const SanitizeMode mode) -> PartSelection {
    auto new_parts = part_selection::part_vector_t {};

    for (const auto &part : parts) {
        if (const auto new_part = find_sanitized_part(part, cache, mode)) {
            new_parts.push_back(new_part.value());
        }
    }

    return PartSelection {std::move(new_parts)};
}

}  // namespace

auto is_sanitized(segment_part_t segment_part, const Layout &layout,
                  const CollisionIndex &cache) -> bool {
    if (!is_inserted(segment_part.segment.wire_id)) {
        return true;
    }

    const auto line = get_line(layout, segment_part);
    return !cache.is_wires_crossing(line.p0) && !cache.is_wires_crossing(line.p1);
}

auto sanitize_part(segment_part_t segment_part, const Layout &layout,
                   const CollisionIndex &cache, SanitizeMode mode) -> segment_part_t {
    if (!is_inserted(segment_part.segment.wire_id)) {
        return segment_part;
    }

    const auto full_line = get_line(layout, segment_part.segment);
    const auto crossing_cache = CrossingCache {cache, full_line};

    if (const auto new_part =
            find_sanitized_part(segment_part.part, crossing_cache, mode)) {
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
                        const Layout &layout, const CollisionIndex &cache,
                        SanitizeMode &mode) -> std::optional<PartSelection> {
    if (!is_inserted(segment.wire_id)) {
        return std::nullopt;
    }

    const auto full_line = get_line(layout, segment);
    const auto crossing_cache = CrossingCache {cache, full_line};

    if (is_colliding(parts, crossing_cache)) {
        return find_sanitized_parts(parts, crossing_cache, mode);
    }

    return std::nullopt;
}

}  // namespace

auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionIndex &cache, SanitizeMode mode) -> void {
    std::vector<segment_t> to_delete {};

    for (const auto &entry : selection.selected_segments()) {
        const auto &[segment, parts] = entry;

        if (auto new_parts = new_sanitize_parts(segment, parts, layout, cache, mode)) {
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
