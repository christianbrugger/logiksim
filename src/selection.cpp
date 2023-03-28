
#include "selection.h"

#include "geometry.h"

namespace logicsim {

auto segment_selection_t::format() const -> std::string {
    return fmt::format("{}-{}", begin, end);
}

auto detail::selection::map_key_t::format() const -> std::string {
    return fmt::format("{}-{}", element_key, segment_index);
}

auto Selection::swap(Selection &other) noexcept -> void {
    using std::swap;

    selected_elements_.swap(other.selected_elements_);
    selected_segments_.swap(other.selected_segments_);
}

auto Selection::clear() -> void {
    selected_elements_.clear();
    selected_segments_.clear();
}

auto Selection::format() const -> std::string {
    return fmt::format(
        "Slection(\n"
        "  elements = {},\n"
        "  segments = {},\n"
        ")",
        selected_elements_.values(), selected_segments_.values());
}

auto Selection::empty() const noexcept -> bool {
    return selected_elements_.empty() && selected_segments_.empty();
}

auto Selection::add_element(element_key_t element) -> void {
    selected_elements_.insert(element);
}

auto Selection::remove_element(element_key_t element) -> void {
    selected_elements_.erase(element);
}

auto Selection::toggle_element(element_key_t element) -> void {
    if (is_selected(element)) {
        remove_element(element);
    } else {
        add_element(element);
    }
}

auto Selection::add_segment(element_key_t element_key, segment_index_t segment_index,
                            segment_selection_t selection) -> void {
    const auto key = detail::selection::map_key_t {
        .element_key = element_key,
        .segment_index = segment_index,
    };
    const auto value = detail::selection::map_value_t {selection};

    selected_segments_.insert_or_assign(key, value);
}

auto Selection::remove_segment(element_key_t element, segment_index_t segment_index,
                               segment_selection_t selection) -> void {}

auto Selection::toggle_segment(element_key_t element, segment_index_t segment_index,
                               segment_selection_t selection) -> void {}

auto Selection::is_selected(element_key_t element) const -> bool {
    return selected_elements_.contains(element);
}

auto Selection::selected_elements() const -> std::span<const element_key_t> {
    return selected_elements_.values();
}

auto Selection::selected_segments() const -> std::span<const segment_pair_t> {
    return selected_segments_.values();
}

auto get_segment_begin_end(line_t segment, rect_fine_t selection_rect) {
    const auto ordered_line = order_points(segment);

    if (is_horizontal(segment)) {
        const auto xmin = clamp_to<grid_t::value_type>(selection_rect.p0.x);
        const auto xmax = clamp_to<grid_t::value_type>(selection_rect.p1.x);

        const auto begin = std::clamp(ordered_line.p0.x.value, xmin, xmax);
        const auto end = std::clamp(ordered_line.p1.x.value, xmin, xmax);

        return std::make_pair(begin, end);
    }

    // vertical
    const auto ymin = clamp_to<grid_t::value_type>(selection_rect.p0.y);
    const auto ymax = clamp_to<grid_t::value_type>(selection_rect.p1.y);

    const auto begin = std::clamp(ordered_line.p0.y.value, ymin, ymax);
    const auto end = std::clamp(ordered_line.p1.y.value, ymin, ymax);

    return std::make_pair(begin, end);
}

auto get_segment_selection(line_t segment, rect_fine_t selection_rect)
    -> std::optional<segment_selection_t> {
    const auto [begin, end] = get_segment_begin_end(segment, selection_rect);

    if (begin == end) {
        return std::nullopt;
    }
    return segment_selection_t {begin, end};
}

auto get_selected_segment(line_t segment, segment_selection_t selection) -> line_t {
    if (is_horizontal(segment)) {
        const auto y = segment.p0.y;
        return line_t {point_t {selection.begin, y}, point_t {selection.end, y}};
    }

    // vertical
    const auto x = segment.p0.x;
    return line_t {point_t {x, selection.begin}, point_t {x, selection.end}};
}

auto swap(Selection &a, Selection &b) noexcept -> void {
    a.swap(b);
}

}  // namespace logicsim

template <>
auto std::swap(logicsim::Selection &a, logicsim::Selection &b) noexcept -> void {
    a.swap(b);
}
