#include "layout.h"

#include "algorithm/accumulate.h"
#include "algorithm/fmt_join.h"
#include "allocated_size/trait.h"
#include "geometry/line.h"
#include "geometry/point.h"
#include "geometry/segment_info.h"
#include "vocabulary/decoration_layout_data.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/placed_element.h"
#include "vocabulary/segment_part.h"

#include <algorithm>

namespace logicsim {

//
// Layout
//

Layout::Layout(circuit_id_t circuit_id) : circuit_id_ {circuit_id} {}

auto Layout::allocated_size() const -> std::size_t {
    return get_allocated_size(logicitems_) +  //
           get_allocated_size(wires_) +       //
           get_allocated_size(decorations_);
}

auto Layout::format() const -> std::string {
    auto inner_logicitems = std::string {};
    auto inner_wires = std::string {};
    auto inner_decorations = std::string {};

    if (!logicitems_.empty()) {
        const auto format_single = [&](logicitem_id_t logicitem_id) {
            return format_logicitem(*this, logicitem_id);
        };
        const auto lines = fmt_join(",\n  ", logicitem_ids(*this), "{}", format_single);
        inner_logicitems = fmt::format(": [\n  {}\n]", lines);
    }

    if (!wires_.empty()) {
        const auto format_single = [&](wire_id_t wire_id) {
            return format_wire(*this, wire_id);
        };
        const auto lines = fmt_join(",\n  ", wire_ids(*this), "{}", format_single);
        inner_wires = fmt::format(", [\n  {}\n]", lines);
    }

    if (!decorations_.empty()) {
        const auto format_single = [&](decoration_id_t decoration_id) {
            return format_decoration(*this, decoration_id);
        };
        const auto lines = fmt_join(",\n  ", decoration_ids(*this), "{}", format_single);
        inner_decorations = fmt::format(", [\n  {}\n]", lines);
    }

    return fmt::format("<Layout with {} logic items, {} wires, {} decorations{}{}{}>",
                       logicitems_.size(), wires_.size(), decorations_.size(),
                       inner_logicitems, inner_wires, inner_decorations);
}

auto Layout::normalize() -> void {
    logicitems_.normalize();
    wires_.normalize();
    decorations_.normalize();
}

auto Layout::empty() const -> bool {
    return logicitems_.empty() && wires_.empty() && decorations_.empty();
}

auto Layout::size() const -> std::size_t {
    return logicitems_.size() + wires_.size() + decorations_.size();
}

auto Layout::circuit_id() const -> circuit_id_t {
    return circuit_id_;
}

auto Layout::logicitems() -> layout::LogicItemStore & {
    return logicitems_;
}

auto Layout::logicitems() const -> const layout::LogicItemStore & {
    return logicitems_;
}

auto Layout::wires() -> layout::WireStore & {
    return wires_;
}

auto Layout::wires() const -> const layout::WireStore & {
    return wires_;
}

auto Layout::decorations() -> layout::DecorationStore & {
    return decorations_;
}

auto Layout::decorations() const -> const layout::DecorationStore & {
    return decorations_;
}

//
// Free functions
//

auto logicitem_ids(const Layout &layout) -> range_extended_t<logicitem_id_t> {
    return range_extended<logicitem_id_t>(layout.logicitems().size());
}

auto wire_ids(const Layout &layout) -> range_extended_t<wire_id_t> {
    return range_extended<wire_id_t>(layout.wires().size());
}

auto decoration_ids(const Layout &layout) -> range_extended_t<decoration_id_t> {
    return range_extended<decoration_id_t>(layout.decorations().size());
}

auto inserted_wire_ids(const Layout &layout) -> range_extended_t<wire_id_t> {
    constexpr static auto first = std::size_t {first_inserted_wire_id};
    return range_extended<wire_id_t>(first, std::max(first, layout.wires().size()));
}

auto is_id_valid(logicitem_id_t logicitem_id, const Layout &layout) -> bool {
    return logicitem_id >= logicitem_id_t {0} &&
           std::size_t {logicitem_id} < layout.logicitems().size();
}

auto is_id_valid(wire_id_t wire_id, const Layout &layout) -> bool {
    return wire_id >= wire_id_t {0} && std::size_t {wire_id} < layout.wires().size();
}

auto is_id_valid(decoration_id_t decoration_id, const Layout &layout) -> bool {
    return decoration_id >= decoration_id_t {0} &&
           std::size_t {decoration_id} < layout.decorations().size();
}

auto is_segment_valid(segment_t segment, const Layout &layout) -> bool {
    if (!is_id_valid(segment.wire_id, layout)) {
        return false;
    };

    assert(segment.segment_index >= segment_index_t {0});
    return std::size_t {segment.segment_index} <
           layout.wires().segment_tree(segment.wire_id).size();
}

auto is_segment_part_valid(segment_part_t segment_part, const Layout &layout) -> bool {
    if (!is_segment_valid(segment_part.segment, layout)) {
        return false;
    }
    return segment_part.part.end <= to_part(get_line(layout, segment_part.segment)).end;
}

auto get_uninserted_logicitem_count(const Layout &layout) -> std::size_t {
    const auto count = std::ranges::count_if(
        logicitem_ids(layout),
        [&](const auto &logicitem_id) { return !is_inserted(layout, logicitem_id); });

    return gsl::narrow_cast<std::size_t>(count);
}

auto get_inserted_logicitem_count(const Layout &layout) -> std::size_t {
    const auto count = std::ranges::count_if(
        logicitem_ids(layout),
        [&](const auto &logicitem_id) { return is_inserted(layout, logicitem_id); });

    return gsl::narrow_cast<std::size_t>(count);
}

auto get_uninserted_decoration_count(const Layout &layout) -> std::size_t {
    const auto count = std::ranges::count_if(
        decoration_ids(layout),
        [&](const auto &decoration_id) { return !is_inserted(layout, decoration_id); });

    return gsl::narrow_cast<std::size_t>(count);
}

auto get_inserted_decoration_count(const Layout &layout) -> std::size_t {
    const auto count = std::ranges::count_if(
        decoration_ids(layout),
        [&](const auto &decoration_id) { return is_inserted(layout, decoration_id); });

    return gsl::narrow_cast<std::size_t>(count);
}

auto get_segment_count(const Layout &layout) -> std::size_t {
    return accumulate(wire_ids(layout), std::size_t {0}, [&](wire_id_t wire_id) {
        return layout.wires().segment_tree(wire_id).size();
    });
}

auto get_inserted_segment_count(const Layout &layout) -> std::size_t {
    return accumulate(inserted_wire_ids(layout), std::size_t {0}, [&](wire_id_t wire_id) {
        return layout.wires().segment_tree(wire_id).size();
    });
}

auto format_stats(const Layout &layout) -> std::string {
    const auto logicitem_count = layout.logicitems().size();
    const auto segment_count = get_segment_count(layout);
    const auto decoration_count = layout.decorations().size();

    return fmt::format(
        "Layout with {} logic items, {} wire segments and {} decorations.\n",
        logicitem_count, segment_count, decoration_count);
}

auto format_logicitem(const Layout &layout, logicitem_id_t logicitem_id) -> std::string {
    return fmt::format("<LogicItem {}: {}x{} {}, {}, {}, {}>", logicitem_id,
                       layout.logicitems().input_count(logicitem_id),
                       layout.logicitems().output_count(logicitem_id),
                       layout.logicitems().type(logicitem_id),
                       layout.logicitems().display_state(logicitem_id),
                       layout.logicitems().position(logicitem_id),
                       layout.logicitems().orientation(logicitem_id));
}

auto format_wire(const Layout &layout, wire_id_t wire_id) -> std::string {
    return fmt::format("<Wire {}: {}>", wire_id, layout.wires().segment_tree(wire_id));
}

auto format_decoration(const Layout &layout,
                       decoration_id_t decoration_id) -> std::string {
    const auto type = layout.decorations().type(decoration_id);

    using enum DecorationType;
    const auto attr_str =
        type == text_element
            ? fmt::format(" \"{}\"",
                          layout.decorations().attrs_text_element(decoration_id))
            : std::string {};

    return fmt::format("<Decoration {}: {}x{} {} {}{}>", decoration_id,
                       layout.decorations().width(decoration_id),
                       layout.decorations().height(decoration_id), type,
                       layout.decorations().position(decoration_id), attr_str);
}

auto is_inserted(const Layout &layout, logicitem_id_t logicitem_id) -> bool {
    return is_inserted(layout.logicitems().display_state(logicitem_id));
}

auto is_inserted(const Layout &layout, decoration_id_t decoration_id) -> bool {
    return is_inserted(layout.decorations().display_state(decoration_id));
}

auto is_wire_empty(const Layout &layout, const wire_id_t wire_id) -> bool {
    return layout.wires().segment_tree(wire_id).empty();
}

auto get_segment_info(const Layout &layout, segment_t segment) -> segment_info_t {
    return layout.wires().segment_tree(segment.wire_id).info(segment.segment_index);
}

auto get_segment_point_type(const Layout &layout, segment_t segment,
                            point_t position) -> SegmentPointType {
    const auto info = get_segment_info(layout, segment);
    return get_segment_point_type(info, position);
}

auto get_segment_valid_parts(const Layout &layout,
                             segment_t segment) -> const PartSelection & {
    return layout.wires()
        .segment_tree(segment.wire_id)
        .valid_parts(segment.segment_index);
}

auto get_line(const Layout &layout, segment_t segment) -> ordered_line_t {
    return get_segment_info(layout, segment).line;
}

auto get_line(const Layout &layout, segment_part_t segment_part) -> ordered_line_t {
    const auto full_line = get_line(layout, segment_part.segment);
    return to_line(full_line, segment_part.part);
}

auto has_segments(const Layout &layout) -> bool {
    return std::ranges::any_of(wire_ids(layout), [&](wire_id_t wire_id) {
        return !layout.wires().segment_tree(wire_id).empty();
    });
}

auto moved_layout(Layout layout, int delta_x, int delta_y) -> std::optional<Layout> {
    // logic items
    for (const auto logicitem_id : logicitem_ids(layout)) {
        const auto position = layout.logicitems().position(logicitem_id);

        if (!is_representable(position, delta_x, delta_y)) {
            return std::nullopt;
        }

        const auto new_position = add_unchecked(position, delta_x, delta_y);
        layout.logicitems().set_position(logicitem_id, new_position);
    }

    // wires
    for (const auto wire_id : wire_ids(layout)) {
        auto &tree = layout.wires().modifiable_segment_tree(wire_id);

        for (const auto segment_index : tree.indices()) {
            auto info = segment_info_t {tree.info(segment_index)};

            if (!is_representable(info.line, delta_x, delta_y)) {
                return std::nullopt;
            }

            info.line = add_unchecked(info.line, delta_x, delta_y);
            tree.update_segment(segment_index, info);
        }
    }

    // decorations
    for (const auto decoration_id : decoration_ids(layout)) {
        const auto position = layout.decorations().position(decoration_id);

        if (!is_representable(position, delta_x, delta_y)) {
            return std::nullopt;
        }

        const auto new_position = add_unchecked(position, delta_x, delta_y);
        layout.decorations().set_position(decoration_id, new_position);
    }

    return std::optional {std::move(layout)};
}

auto to_layout_calculation_data(const Layout &layout, logicitem_id_t logicitem_id)
    -> layout_calculation_data_t {
    return layout::to_layout_calculation_data(layout.logicitems(), logicitem_id);
}

auto to_decoration_layout_data(const Layout &layout, decoration_id_t decoration_id)
    -> decoration_layout_data_t {
    return layout::to_decoration_layout_data(layout.decorations(), decoration_id);
}

auto to_logicitem_definition(const Layout &layout,
                             logicitem_id_t logicitem_id) -> LogicItemDefinition {
    return layout::to_logicitem_definition(layout.logicitems(), logicitem_id);
}

auto to_decoration_definition(const Layout &layout,
                              decoration_id_t decoration_id) -> DecorationDefinition {
    return layout::to_decoration_definition(layout.decorations(), decoration_id);
}

auto to_placed_element(const Layout &layout,
                       logicitem_id_t logicitem_id) -> PlacedElement {
    return PlacedElement {
        .definition = to_logicitem_definition(layout, logicitem_id),
        .position = layout.logicitems().position(logicitem_id),
    };
}

auto get_display_states(const Layout &layout, segment_part_t segment_part)
    -> std::pair<display_state_t, display_state_t> {
    using enum display_state_t;

    // aggregates
    if (is_temporary(segment_part.segment.wire_id)) {
        return std::make_pair(temporary, temporary);
    }
    if (is_colliding(segment_part.segment.wire_id)) {
        return std::make_pair(colliding, colliding);
    }

    const auto &tree = layout.wires().segment_tree(segment_part.segment.wire_id);

    // check valid parts
    for (const auto valid_part : tree.valid_parts(segment_part.segment.segment_index)) {
        // parts can not touch or overlap, so we can return early
        if (a_inside_b(segment_part.part, valid_part)) {
            return std::make_pair(valid, valid);
        }
        if (a_overlaps_any_of_b(segment_part.part, valid_part)) {
            return std::make_pair(valid, normal);
        }
    }
    return std::make_pair(normal, normal);
}

auto get_insertion_modes(const Layout &layout, segment_part_t segment_part)
    -> std::pair<InsertionMode, InsertionMode> {
    const auto display_states = get_display_states(layout, segment_part);

    return std::make_pair(to_insertion_mode(display_states.first),
                          to_insertion_mode(display_states.second));
}

auto all_normal_display_state(const Layout &layout) -> bool {
    const auto logicitem_normal = [&](const logicitem_id_t &logicitem_id) {
        return layout.logicitems().display_state(logicitem_id) == display_state_t::normal;
    };
    const auto wire_normal = [&](const wire_id_t &wire_id) {
        return std::ranges::all_of(layout.wires().segment_tree(wire_id).valid_parts(),
                                   &PartSelection::empty);
    };
    const auto decoration_normal = [&](const decoration_id_t &decoration_id) {
        return layout.decorations().display_state(decoration_id) ==
               display_state_t::normal;
    };

    return layout.wires().segment_tree(temporary_wire_id).empty() &&
           layout.wires().segment_tree(colliding_wire_id).empty() &&
           std::ranges::all_of(logicitem_ids(layout), logicitem_normal) &&
           std::ranges::all_of(decoration_ids(layout), decoration_normal) &&
           std::ranges::all_of(inserted_wire_ids(layout), wire_normal);
}

}  // namespace logicsim
