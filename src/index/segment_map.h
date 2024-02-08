#ifndef LOGICSIM_INDEX_SEGMENT_MAP_H
#define LOGICSIM_INDEX_SEGMENT_MAP_H

#include "vocabulary/orientation.h"
#include "vocabulary/point.h"
#include "vocabulary/segment.h"

#include <ankerl/unordered_dense.h>

#include <array>
#include <optional>
#include <utility>

namespace logicsim {

struct ordered_line_t {};

class Layout;
class Selection;

namespace segment_index {

[[nodiscard]] auto to_index(orientation_t orientation)
    -> std::underlying_type<orientation_t>::type;

struct adjacent_segments_t {
   public:
    using value_type = segment_t;

   public:
    // orientation: right, left, up, down
    std::array<segment_t, 4> segments;

   public:
    [[nodiscard]] auto at(orientation_t orientation) const -> segment_t;
    [[nodiscard]] auto has(orientation_t orientation) const -> bool;

    [[nodiscard]] auto count() const -> std::ptrdiff_t;
};

using mergable_t = std::pair<segment_t, segment_t>;

[[nodiscard]] auto get_mergeable_segments(const adjacent_segments_t& segments)
    -> std::optional<mergable_t>;

using map_t = ankerl::unordered_dense::map<point_t, adjacent_segments_t>;

}  // namespace segment_index

/*
class SegmentMap {
   public:
    using value_type = segment_index::adjacent_segments_t;
    using map_t = segment_index::map_t;

   public:
    auto add_segment(segment_t segment, ordered_line_t line) -> void {
        add_point(line.p0, segment, to_orientation_p0(line));
        add_point(line.p1, segment, to_orientation_p1(line));
    }

    // [](point_t point, std::array<segment_t,4> segments, int count) {}
    template <typename Func>
    auto iter_crosspoints(Func callback) const -> void {
        for (const auto& [point, segments] : map_.values()) {
            const auto count = count_points(segments);

            if (count >= 3) {
                callback(point, segments, count);
            }
        }
    }

    [[nodiscard]] auto adjacent_segments() const -> std::vector<mergable_t> {
        auto result = std::vector<mergable_t> {};

        for (const auto& [point, segments] : map_.values()) {
            if (const auto adjacent = to_adcacent_segment(segments)) {
                result.push_back(*adjacent);
            }
        }

        return result;
    }

   private:
    auto add_point(point_t point, segment_t segment, orientation_t orientation) -> void {
        const auto index = this->index(orientation);

        const auto it = map_.find(point);

        if (it != map_.end()) {
            if (it->second.at(index) != null_segment) [[unlikely]] {
                throw std::runtime_error("entry already exists in SegmentEndpointMap");
            }

            it->second.at(index) = segment;
        } else {
            auto value = value_t {null_segment, null_segment, null_segment, null_segment};
            value.at(index) = segment;

            map_.emplace(point, value);
        }
    }

   private:
    map_t map_ {};
};

auto build_endpoint_map(const Layout& layout, const Selection& selection)
    -> SegmentEndpointMap {
    auto map = SegmentEndpointMap {};

    for (const auto& [segment, parts] : selection.selected_segments()) {
        const auto full_line = get_line(layout, segment);

        if (!is_temporary(segment.wire_id)) {
            throw std::runtime_error("can only merge temporary segments");
        }
        if (parts.size() != 1 || to_part(full_line) != parts.front()) [[unlikely]] {
            throw std::runtime_error("selection cannot contain partially selected lines");
        }

        map.add_segment(segment, full_line);
    }

    return map;
}
*/

}  // namespace logicsim

#endif
