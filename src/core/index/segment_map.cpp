#include "core/index/segment_map.h"

#include "core/algorithm/to_underlying.h"
#include "core/geometry/orientation.h"
#include "core/layout.h"
#include "core/selection.h"
#include "core/vocabulary/ordered_line.h"

#include <stdexcept>

namespace logicsim {

namespace segment_map {

auto to_index(orientation_t orientation) -> std::underlying_type_t<orientation_t> {
    if (orientation == orientation_t::undirected) [[unlikely]] {
        throw std::runtime_error("not supported");
    }

    return to_underlying(orientation);
}

auto adjacent_segments_t::at(orientation_t orientation) const -> const segment_t& {
    return segments.at(to_index(orientation));
}

auto adjacent_segments_t::at(orientation_t orientation) -> segment_t& {
    return segments.at(to_index(orientation));
}

auto adjacent_segments_t::has(orientation_t orientation) const -> bool {
    return at(orientation) != null_segment;
}

auto adjacent_segments_t::count() const -> std::ptrdiff_t {
    return std::ranges::count_if(segments, &segment_t::operator bool);
}

auto get_mergeable_segments(const adjacent_segments_t& segments)
    -> std::optional<mergable_t> {
    using enum orientation_t;

    // if point has more than 2 segments, its not mergable, as it has a cross-point
    if (segments.count() != 2) {
        return std::nullopt;
    }

    if (segments.has(left) && segments.has(right)) {
        return mergable_t {segments.at(left), segments.at(right)};
    }

    if (segments.has(up) && segments.has(down)) {
        return mergable_t {segments.at(up), segments.at(down)};
    }

    return std::nullopt;
}

}  // namespace segment_map

//
// Segment Map
//

namespace segment_map {

namespace {

auto add_point(map_t& map, point_t point, segment_t segment,
               orientation_t orientation) -> void {
    if (const auto it = map.find(point); it != map.end()) {
        // allow overwriting of segments, as temporary wires can be of any arrangement
        // throws for orientation_t::undirected
        it->second.at(orientation) = segment;
    } else {
        auto value = adjacent_segments_t {};
        value.at(orientation) = segment;
        Expects(map.emplace(point, value).second);
    }
}

}  // namespace

}  // namespace segment_map

auto logicsim::SegmentMap::add_segment(segment_t segment, ordered_line_t line) -> void {
    segment_map::add_point(map_, line.p0, segment, to_orientation_p0(line));
    segment_map::add_point(map_, line.p1, segment, to_orientation_p1(line));
}

auto SegmentMap::segments() const -> const map_t& {
    return map_;
}

//
// Free Functions
//

auto adjacent_segments(const SegmentMap& segment_map)
    -> std::vector<segment_map::mergable_t> {
    using namespace segment_map;

    auto result = std::vector<mergable_t> {};

    for (const auto& [point, segments] : segment_map.segments()) {
        if (const auto adjacent = get_mergeable_segments(segments)) {
            result.push_back(*adjacent);
        }
    }

    return result;
}

auto build_endpoint_map(const Layout& layout, const Selection& selection) -> SegmentMap {
    auto map = SegmentMap {};

    for (const auto& [segment, parts] : selection.selected_segments()) {
        const auto full_line = get_line(layout, segment);

        // TODO move ???
        if (!is_temporary(segment.wire_id)) {
            throw std::runtime_error("can only merge temporary segments");
        }
        // TODO move ???
        if (parts.size() != 1 || to_part(full_line) != parts.front()) [[unlikely]] {
            throw std::runtime_error("selection cannot contain partially selected lines");
        }

        map.add_segment(segment, full_line);
    }

    return map;
}

}  // namespace logicsim
