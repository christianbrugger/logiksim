#include "core/geometry/layout_geometry.h"

#include "core/geometry/rect.h"
#include "core/layout.h"
#include "core/layout_info.h"
#include "core/vocabulary/rect.h"

#include <numeric>
#include <ranges>

namespace logicsim {

namespace {

const auto enclosing_rect_opt = [](const std::optional<rect_t>& a,
                                   const std::optional<rect_t>& b) {
    return enclosing_rect(a, b);
};

const auto enclosing_rect_opt3 = [](const std::optional<rect_t>& a,
                                    const std::optional<rect_t>& b,
                                    const std::optional<rect_t>& c) {
    return enclosing_rect(enclosing_rect(a, b), c);
};

}  // namespace

auto bounding_rect_logicitems(const Layout& layout) -> std::optional<rect_t> {
    const auto to_bounding_rect =
        [&](logicitem_id_t logicitem_id) -> std::optional<rect_t> {
        return layout.logicitems().bounding_rect(logicitem_id);
    };

    const auto view =
        logicitem_ids(layout) | std::ranges::views::transform(to_bounding_rect);

    // C++23 use std::ranges::fold_left
    return std::accumulate(view.begin(), view.end(), std::optional<rect_t> {},
                           enclosing_rect_opt);
}

auto bounding_rect_decorations(const Layout& layout) -> std::optional<rect_t> {
    const auto to_bounding_rect =
        [&](decoration_id_t decoration_id) -> std::optional<rect_t> {
        return layout.decorations().bounding_rect(decoration_id);
    };

    const auto view =
        decoration_ids(layout) | std::ranges::views::transform(to_bounding_rect);

    // C++23 use std::ranges::fold_left
    return std::accumulate(view.begin(), view.end(), std::optional<rect_t> {},
                           enclosing_rect_opt);
}

auto bounding_rect_inserted_segments(const Layout& layout) -> std::optional<rect_t> {
    const auto to_bounding_rect = [&](wire_id_t wire_id) -> std::optional<rect_t> {
        return layout.wires().bounding_rect(wire_id);
    };

    const auto view =
        inserted_wire_ids(layout) | std::ranges::views::transform(to_bounding_rect);

    // C++23 use std::ranges::fold_left
    return std::accumulate(view.begin(), view.end(), std::optional<rect_t> {},
                           enclosing_rect_opt);
}

auto bounding_rect_uninserted_segments(const Layout& layout) -> std::optional<rect_t> {
    return enclosing_rect_opt(
        calculate_bounding_rect(layout.wires().segment_tree(temporary_wire_id)),
        calculate_bounding_rect(layout.wires().segment_tree(colliding_wire_id)));
}

auto bounding_rect_segments(const Layout& layout) -> std::optional<rect_t> {
    return enclosing_rect_opt(bounding_rect_inserted_segments(layout),
                              bounding_rect_uninserted_segments(layout));
}

auto bounding_rect(const Layout& layout) -> std::optional<rect_t> {
    return enclosing_rect_opt3(bounding_rect_logicitems(layout),
                               bounding_rect_decorations(layout),
                               bounding_rect_segments(layout));
}

}  // namespace logicsim
