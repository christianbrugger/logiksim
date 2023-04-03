
#include "editable_circuit/selection.h"

#include "circuit.h"
#include "geometry.h"
#include "range.h"

namespace logicsim {

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

auto Selection::add_element(element_id_t element_id) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("added element_id needs to be valid");
    }

    selected_elements_.insert(element_id);
}

auto Selection::remove_element(element_id_t element_id) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("removed element_id needs to be valid");
    }

    selected_elements_.erase(element_id);
}

auto Selection::toggle_element(element_id_t element_id) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("toggled element_id needs to be valid");
    }

    if (is_selected(element_id)) {
        remove_element(element_id);
    } else {
        add_element(element_id);
    }
}

auto Selection::add_segment(segment_t segment, segment_part_t parts) -> void {
    const auto it = selected_segments_.find(segment);
    if (it == selected_segments_.end()) {
        // insert new list
        const auto value = detail::selection::map_value_t {parts};
        bool inserted = selected_segments_.emplace(segment, value).second;
        if (!inserted) [[unlikely]] {
            throw_exception("unable to insert value");
        }
        return;
    }

    auto &entries = it->second;
    if (entries.size() == 0) [[unlikely]] {
        throw_exception("found segment selection with zero selection entries");
    }

    // sort
    entries.push_back(parts);
    std::ranges::sort(entries);

    // merge elements
    auto result = detail::selection::map_value_t {};
    using it_t = detail::selection::map_value_t::iterator;

    transform_combine_while(
        entries, std::back_inserter(result),
        // make state
        [](it_t it) -> segment_part_t { return *it; },
        // combine while
        [](segment_part_t state, it_t it) -> bool { return state.end >= it->begin; },
        // update state
        [](segment_part_t state, it_t it) -> segment_part_t {
            return segment_part_t {state.begin, std::max(state.end, it->end)};
        });

    if (result.size() == 0) [[unlikely]] {
        throw_exception("algorithm result should not be empty");
    }
    entries.swap(result);
}

auto Selection::remove_segment(segment_t segment, segment_part_t removing) -> void {
    const auto it = selected_segments_.find(segment);
    if (it == selected_segments_.end()) {
        return;
    }

    auto &entries = it->second;
    if (entries.size() == 0) [[unlikely]] {
        throw_exception("found segment selection with zero selection entries");
    }

    for (auto i : reverse_range(entries.size())) {
        const auto entry = segment_part_t {entries[i]};
        // SEE 'selection_model.md' for visual cases

        // no overlapp -> keep
        if (entry.begin >= removing.end || entry.end <= removing.begin) {
        }

        // new completely inside -> split
        else if (entry.begin < removing.begin && entry.end > removing.end) {
            entries[i] = segment_part_t {entry.begin, removing.begin};
            entries.emplace_back(removing.end, entry.end);
        }

        // new complete overlapps -> swap & remove
        else if (entry.begin >= removing.begin && entry.end <= removing.end) {
            entries[i] = entries[entries.size() - 1];
            entries.pop_back();
        }

        // right sided overlap -> shrink right
        else if (entry.begin < removing.begin && entry.end > removing.begin
                 && entry.end <= removing.end) {
            entries[i] = segment_part_t {entry.begin, removing.begin};
        }
        // left sided overlap -> shrink left
        else if (entry.begin >= removing.begin && entry.begin < removing.end
                 && entry.end > removing.end) {
            entries[i] = segment_part_t {removing.end, entry.end};
        }

        else {
            throw_exception("unknown case in remove_segment");
        }
    }

    if (entries.empty()) {
        if (!selected_segments_.erase(segment)) {
            throw_exception("unable to delete key");
        }
    }
}

auto Selection::toggle_segment(segment_t segment, segment_part_t selection) -> void {}

auto Selection::is_selected(element_id_t element_id) const -> bool {
    return selected_elements_.contains(element_id);
}

auto Selection::selected_elements() const -> std::span<const element_id_t> {
    return selected_elements_.values();
}

auto Selection::selected_segments() const -> std::span<const segment_pair_t> {
    return selected_segments_.values();
}

auto Selection::selected_segments(segment_t segment) const
    -> std::span<const segment_part_t> {
    const auto it = selected_segments_.find(segment);
    if (it == selected_segments_.end()) {
        return {};
    }

    auto &entries = it->second;
    if (entries.size() == 0) [[unlikely]] {
        throw_exception("found segment selection with zero selection entries");
    }

    return it->second;
}

auto get_segment_begin_end(line_t line, rect_fine_t selection_rect) {
    const auto ordered_line = order_points(line);

    if (is_horizontal(line)) {
        const auto xmin = clamp_to<grid_t::value_type>(std::floor(selection_rect.p0.x));
        const auto xmax = clamp_to<grid_t::value_type>(std::ceil(selection_rect.p1.x));

        const auto begin = std::clamp(ordered_line.p0.x.value, xmin, xmax);
        const auto end = std::clamp(ordered_line.p1.x.value, xmin, xmax);

        return std::make_pair(begin, end);
    }

    // vertical
    const auto ymin = clamp_to<grid_t::value_type>(std::floor(selection_rect.p0.y));
    const auto ymax = clamp_to<grid_t::value_type>(std::ceil(selection_rect.p1.y));

    const auto begin = std::clamp(ordered_line.p0.y.value, ymin, ymax);
    const auto end = std::clamp(ordered_line.p1.y.value, ymin, ymax);

    return std::make_pair(begin, end);
}

auto get_segment_part(line_t line) -> segment_part_t {
    const auto ordered_line = order_points(line);

    if (is_horizontal(line)) {
        return segment_part_t {ordered_line.p0.x, ordered_line.p1.x};
    }
    return segment_part_t {ordered_line.p0.y, ordered_line.p1.y};
}

auto get_segment_part(line_t line, rect_fine_t selection_rect)
    -> std::optional<segment_part_t> {
    const auto [begin, end] = get_segment_begin_end(line, selection_rect);

    if (begin == end) {
        return std::nullopt;
    }
    return segment_part_t {begin, end};
}

// TODO rename
auto get_selected_segment(line_t segment, segment_part_t selection) -> line_t {
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

namespace logicsim {

//
// Updates
//

auto Selection::handle(editable_circuit::info_message::ElementDeleted message) -> void {
    remove_element(message.element_id);
}

auto Selection::handle(editable_circuit::info_message::ElementUpdated message) -> void {
    const auto count = selected_elements_.erase(message.old_element_id);
    if (count > 0) {
        const auto inserted = selected_elements_.insert(message.new_element_id).second;

        if (!inserted) [[unlikely]] {
            throw_exception("element already existed");
        }
    }
}

auto Selection::handle(editable_circuit::info_message::SegmentDeleted message) -> void {
    selected_segments_.erase(message.segment);
}

auto Selection::handle(editable_circuit::info_message::SegmentUpdated message) -> void {
    const auto it = selected_segments_.find(message.old_segment);

    if (it != selected_segments_.end()) {
        auto parts = detail::selection::map_value_t {std::move(it->second)};
        selected_segments_.erase(it);

        const auto inserted
            = selected_segments_.emplace(message.new_segment, std::move(parts)).second;

        if (!inserted) [[unlikely]] {
            throw_exception("line segment already existed");
        }
    }
}

auto Selection::submit(editable_circuit::InfoMessage message) -> void {
    using namespace editable_circuit::info_message;

    if (const auto pointer = std::get_if<ElementDeleted>(&message)) {
        handle(*pointer);
        return;
    }
    if (const auto pointer = std::get_if<ElementUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (const auto pointer = std::get_if<SegmentDeleted>(&message)) {
        handle(*pointer);
        return;
    }
    if (const auto pointer = std::get_if<SegmentUpdated>(&message)) {
        handle(*pointer);
        return;
    }
}

//
// validation
//

namespace {

using namespace detail::selection;

auto check_and_remove_element(elements_set_t &element_set,
                              const Schematic::ConstElement element) -> void {
    if (!element.is_placeholder()) {
        element_set.erase(element.element_id());
    } else if (element_set.contains(element.element_id())) [[unlikely]] {
        throw_exception("selection contains placeholder");
    }
}

auto check_segment_parts_destructive(line_t line, map_value_t &parts) -> void {
    const auto sorted_line = order_points(line);

    // part of line?
    for (const auto part : parts) {
        if (is_horizontal(line)) {
            if (part.begin < sorted_line.p0.x || part.end > sorted_line.p1.x)
                [[unlikely]] {
                throw_exception("part is not part of line");
            }
        } else {
            if (part.begin < sorted_line.p0.y || part.end > sorted_line.p1.y)
                [[unlikely]] {
                throw_exception("part is not part of line");
            }
        }
    }

    // overlapping or touching?
    std::ranges::sort(parts);
    const auto part_overlapping = [](segment_part_t part0, segment_part_t part1) -> bool {
        return part0.end >= part1.begin;
    };
    if (std::ranges::adjacent_find(parts, part_overlapping) != parts.end()) {
        throw_exception("some parts are overlapping");
    }
}

auto check_and_remove_segments(detail::selection::segment_map_t &segment_map,
                               const element_id_t element_id,
                               const SegmentTree &segment_tree) -> void {
    for (const auto segment_index : segment_tree.indices()) {
        const auto key = segment_t {element_id, segment_index};
        const auto it = segment_map.find(key);

        if (it != segment_map.end()) {
            const auto line = segment_tree.segment(segment_index).line;
            check_segment_parts_destructive(line, it->second);
            segment_map.erase(it);
        }
    }
}

auto check_wire_not_in_segments(element_id_t element_id, const SegmentTree &tree,
                                const detail::selection::segment_map_t &segment_map)
    -> void {
    for (const auto index : tree.indices()) {
        const auto key = segment_t(element_id, index);

        if (segment_map.contains(key)) [[unlikely]] {
            throw_exception(
                "segment tree should either be in elements or segments part of the "
                "selection, but not both.");
        }
    }
}

}  // namespace

auto Selection::validate(const Circuit &circuit) const -> void {
    auto element_set = detail::selection::elements_set_t {selected_elements_};
    auto segment_map = detail::selection::segment_map_t {selected_segments_};

    for (const auto element_id : element_set) {
        check_wire_not_in_segments(element_id, circuit.layout().segment_tree(element_id),
                                   segment_map);
    }

    // elements
    for (const auto element : circuit.schematic().elements()) {
        check_and_remove_element(element_set, element);
    }
    if (!element_set.empty()) [[unlikely]] {
        throw_exception("selection contains elements that don't exist anymore");
    }

    // segments
    for (const auto element : circuit.schematic().elements()) {
        if (element.is_wire()) {
            check_and_remove_segments(
                segment_map, element.element_id(),
                circuit.layout().segment_tree(element.element_id()));
        }
    }
    if (!segment_map.empty()) [[unlikely]] {
        throw_exception("selection contains segments that don't exist anymore");
    }
}

}  // namespace logicsim