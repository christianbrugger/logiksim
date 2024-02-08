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

struct ordered_line_t;

class Layout;
class Selection;

namespace segment_map {

[[nodiscard]] auto to_index(orientation_t orientation)
    -> std::underlying_type<orientation_t>::type;

struct adjacent_segments_t {
   public:
    using value_type = segment_t;

   public:
    // orientation: right, left, up, down
    std::array<segment_t, 4> segments {null_segment, null_segment, null_segment,
                                       null_segment};

   public:
    [[nodiscard]] auto at(orientation_t orientation) const -> const segment_t&;
    [[nodiscard]] auto at(orientation_t orientation) -> segment_t&;
    [[nodiscard]] auto has(orientation_t orientation) const -> bool;

    [[nodiscard]] auto count() const -> std::ptrdiff_t;
};

using mergable_t = std::pair<segment_t, segment_t>;

[[nodiscard]] auto get_mergeable_segments(const adjacent_segments_t& segments)
    -> std::optional<mergable_t>;

using map_t = ankerl::unordered_dense::map<point_t, adjacent_segments_t>;

}  // namespace segment_map

class SegmentMap {
   public:
    using value_type = segment_map::adjacent_segments_t;
    using map_t = segment_map::map_t;
    using mergable_t = segment_map::mergable_t;

   public:
    auto add_segment(segment_t segment, ordered_line_t line) -> void;
    [[nodiscard]] auto segments() const -> const map_t&;

   private:
    map_t map_ {};
};

//
// Free Functions
//

[[nodiscard]] auto build_endpoint_map(const Layout& layout, const Selection& selection)
    -> SegmentMap;

[[nodiscard]] auto adjacent_segments(const SegmentMap& segment_map)
    -> std::vector<segment_map::mergable_t>;

template <typename Func>
auto iter_crosspoints(const SegmentMap& segment_map, Func callback) -> void;

//
// Implementation
//

// [](point_t point, std::array<segment_t,4> segments, int count) {}
template <typename Func>
auto iter_crosspoints(const SegmentMap& segment_map, Func callback) -> void {
    for (const auto& [point, segments] : segment_map.segments()) {
        const auto count = count_points(segments);

        if (count >= 3) {
            callback(point, segments, count);
        }
    }
}

}  // namespace logicsim

#endif
