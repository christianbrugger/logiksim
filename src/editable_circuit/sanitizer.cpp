
#include "editable_circuit/sanitizer.h"

#include "editable_circuit/caches/collision_cache.h"
#include "editable_circuit/selection.h"
#include "geometry.h"
#include "layout.h"
#include "range.h"

#include <folly/small_vector.h>

#include <queue>
#include <utility>

namespace logicsim {

template <>
auto format(SanitizeMode mode) -> std::string {
    switch (mode) {
        case SanitizeMode::shrink:
            return "shrink";
        case SanitizeMode::expand:
            return "expand";
    }
    throw_exception("unknown SanitizeMode");
}

namespace {

class CrossingCache {
   public:
    CrossingCache(const CollisionCache &cache, ordered_line_t full_line)
        : collision_cache_ {cache}, full_line_ {full_line} {}

    auto is_colliding(point_t point) const -> bool {
        return collision_cache_.is_wires_crossing(point);
    }

    auto is_colliding(offset_t offset) const -> bool {
        const auto point = to_point(full_line_, offset);
        return is_colliding(point);
    }

    auto max_offset() const -> offset_t {
        return to_part(full_line_).end;
    }

   private:
    const CollisionCache &collision_cache_;
    const ordered_line_t full_line_;
};

using part_vector_t = Selection::part_vector_t;

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

auto find_higher(offset_t offset, const CrossingCache &cache, offset_t limit)
    -> offset_t {
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
                          const SanitizeMode mode) -> part_vector_t {
    auto new_parts = part_vector_t {};

    for (const auto &part : parts) {
        if (const auto new_part = find_sanitized_part(part, cache, mode)) {
            new_parts.push_back(new_part.value());
        }
    }
    if (mode == SanitizeMode::expand) {
        sort_and_merge_parts(new_parts);
    }
    return new_parts;
}

}  // namespace

auto sanitize_part(segment_part_t segment_part, const Layout &layout,
                   const CollisionCache &cache, SanitizeMode mode) -> segment_part_t {
    const auto full_line = get_line(layout, segment_part.segment);
    const auto crossing_cache = CrossingCache {cache, full_line};

    if (const auto new_part
        = find_sanitized_part(segment_part.part, crossing_cache, mode)) {
        return segment_part_t {segment_part.segment, new_part.value()};
    }
    return null_segment_part;
}

auto sanitize_selection(Selection &selection, const Layout &layout,
                        const CollisionCache &cache, SanitizeMode mode) -> void {
    std::vector<segment_t> to_delete {};

    for (const auto &entry : selection.selected_segments()) {
        const auto segment = entry.first;
        const auto full_line = get_line(layout, segment);
        const auto crossing_cache = CrossingCache {cache, full_line};

        if (is_colliding(entry.second, crossing_cache)) {
            auto new_segments = find_sanitized_parts(entry.second, crossing_cache, mode);
            if (new_segments.empty()) {
                to_delete.push_back(segment);
            } else {
                selection.set_selection(segment, std::move(new_segments));
            }
        }
    }

    for (auto &segment : to_delete) {
        selection.set_selection(segment, {});
    }
}

}  // namespace logicsim