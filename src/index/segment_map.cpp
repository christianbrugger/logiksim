#include "index/segment_map.h"

#include <stdexcept>

namespace logicsim {

namespace segment_index {

auto to_index(orientation_t orientation) -> std::underlying_type<orientation_t>::type {
    if (orientation == orientation_t::undirected) [[unlikely]] {
        throw std::runtime_error("not supported");
    }

    return static_cast<std::underlying_type<orientation_t>::type>(orientation);
}

auto adjacent_segments_t::at(orientation_t orientation) const -> segment_t {
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

    // if point has more than 3 segments, its not mergable, as it has a cross-point
    if (segments.count() != 2) {
        return std::nullopt;
    }

    if (segments.has(left) && segments.has(right)) {
        return mergable_t {segments.at(left), segments.at(right)};
    }

    else if (segments.has(up) && segments.has(down)) {
        return mergable_t {segments.at(up), segments.at(down)};
    }

    return std::nullopt;
}

}  // namespace segment_index

}  // namespace logicsim
