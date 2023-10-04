#include "geometry/part_list.h"

namespace logicsim {

auto sort_and_validate_segment_parts(std::span<part_t> parts, ordered_line_t line)
    -> void {
    // part inside line
    for (const auto part : parts) {
        if (!is_part_valid(part, line)) [[unlikely]] {
            std::domain_error("part is not part of line");
        }
    }

    // parts overlapping or touching?
    std::ranges::sort(parts);
    const auto part_overlapping = [](part_t part0, part_t part1) -> bool {
        return part0.end >= part1.begin;
    };
    if (std::ranges::adjacent_find(parts, part_overlapping) != parts.end()) {
        std::domain_error("some parts are overlapping");
    }
}

auto validate_segment_parts(std::span<const part_t> parts, ordered_line_t line) -> void {
    using parts_vector_t = typename folly::small_vector<part_t, 4>;

    auto copy = parts_vector_t {parts.begin(), parts.end()};
    sort_and_validate_segment_parts(copy, line);
}

}  // namespace logicsim
