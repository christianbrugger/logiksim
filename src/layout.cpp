#include "layout.h"

#include "algorithm/accumulate.h"
#include "algorithm/fmt_join.h"
#include "allocated_size/trait.h"
#include "geometry/line.h"
#include "geometry/point.h"
#include "geometry/segment_info.h"
#include "layout.h"
#include "vocabulary/layout_calculation_data.h"
#include "vocabulary/placed_element.h"
#include "vocabulary/segment_part.h"

namespace logicsim {

//
// Layout
//

Layout::Layout(circuit_id_t circuit_id) : circuit_id_ {circuit_id} {}

auto Layout::allocated_size() const -> std::size_t {
    return get_allocated_size(logic_items_) +  //
           get_allocated_size(wires_);
}

auto Layout::format() const -> std::string {
    auto inner_logic_items = std::string {};
    auto inner_wires = std::string {};

    if (!logic_items_.empty()) {
        const auto format_single = [&](logicitem_id_t logicitem_id) {
            return format_logic_item(*this, logicitem_id);
        };
        const auto lines = fmt_join(",\n  ", logicitem_ids(*this), {}, format_single);
        inner_logic_items = fmt::format(": [\n  {}\n]", lines);
    }

    if (!wires_.empty()) {
        const auto format_single = [&](wire_id_t wire_id) {
            return format_wire(*this, wire_id);
        };
        const auto lines = fmt_join(",\n  ", wire_ids(*this), {}, format_single);
        inner_wires = fmt::format(": [\n  {}\n]", lines);
    }

    return fmt::format("<Layout with {} logic items and {} wires{}>", logic_items_.size(),
                       wires_.size(), inner_logic_items, inner_wires);
}

auto Layout::normalize() -> void {
    logic_items_.normalize();
    wires_.normalize();
}

auto Layout::empty() const -> bool {
    return logic_items_.empty() && wires_.empty();
}

auto Layout::circuit_id() const -> circuit_id_t {
    return circuit_id_;
}

auto Layout::logic_items() -> layout::LogicItemStore & {
    return logic_items_;
}

auto Layout::logic_items() const -> const layout::LogicItemStore & {
    return logic_items_;
}

auto Layout::wires() -> layout::WireStore & {
    return wires_;
}

auto Layout::wires() const -> const layout::WireStore & {
    return wires_;
}

//
// Free functions
//

auto logicitem_ids(const Layout &layout) -> range_extended_t<logicitem_id_t> {
    return range_extended<logicitem_id_t>(layout.logic_items().size());
}

auto wire_ids(const Layout &layout) -> range_extended_t<wire_id_t> {
    return range_extended<wire_id_t>(layout.wires().size());
}

auto inserted_wire_ids(const Layout &layout) -> range_extended_t<wire_id_t> {
    return range_extended<wire_id_t>(std::size_t {first_inserted_wire_id},
                                     layout.wires().size());
}

auto format_stats(const Layout &layout) -> std::string {
    const auto logic_item_count = layout.logic_items().size();
    const auto segment_count = accumulate(
        wire_ids(layout), std::size_t {0},
        [&](wire_id_t wire_id) { return layout.wires().segment_tree(wire_id).size(); });

    return fmt::format("Layout with {} logic items and {} wire segments.\n",
                       logic_item_count, segment_count);
}

auto format_logic_item(const Layout &layout, logicitem_id_t logicitem_id) -> std::string {
    return fmt::format("<LogicItem {}: {}x{} {}, {}, {}, {}", logicitem_id,
                       layout.logic_items().input_count(logicitem_id),
                       layout.logic_items().output_count(logicitem_id),
                       layout.logic_items().type(logicitem_id),
                       layout.logic_items().display_state(logicitem_id),
                       layout.logic_items().position(logicitem_id),
                       layout.logic_items().orientation(logicitem_id));
}

auto format_wire(const Layout &layout, wire_id_t wire_id) -> std::string {
    return fmt::format("<Wire {}: {}", wire_id, layout.wires().segment_tree(wire_id));
}

auto is_inserted(const Layout &layout, logicitem_id_t logicitem_id) -> bool {
    return is_inserted(layout.logic_items().display_state(logicitem_id));
}

auto get_segment_info(const Layout &layout, segment_t segment) -> segment_info_t {
    return layout.wires().segment_tree(segment.wire_id).info(segment.segment_index);
}

auto get_segment_point_type(const Layout &layout, segment_t segment, point_t position)
    -> SegmentPointType {
    const auto info = get_segment_info(layout, segment);
    return get_segment_point_type(info, position);
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
        const auto position = layout.logic_items().position(logicitem_id);

        if (!is_representable(position, delta_x, delta_y)) {
            return std::nullopt;
        }

        const auto new_position = add_unchecked(position, delta_x, delta_y);
        layout.logic_items().set_position(logicitem_id, new_position);
    }

    // wires
    for (const auto wire_id : wire_ids(layout)) {
        auto &tree = layout.wires().modifyable_segment_tree(wire_id);

        for (const auto segment_index : tree.indices()) {
            auto info = segment_info_t {tree.info(segment_index)};

            if (!is_representable(info.line, delta_x, delta_y)) {
                return std::nullopt;
            }

            info.line = add_unchecked(info.line, delta_x, delta_y);
            tree.update_segment(segment_index, info);
        }
    }

    return std::optional {std::move(layout)};
}

auto to_layout_calculation_data(const Layout &layout, logicitem_id_t logicitem_id)
    -> layout_calculation_data_t {
    return layout::to_layout_calculation_data(layout.logic_items(), logicitem_id);
}

auto to_logicitem_definition(const Layout &layout, logicitem_id_t logicitem_id)
    -> ElementDefinition {
    return layout::to_logicitem_definition(layout.logic_items(), logicitem_id);
}

auto to_placed_element(const Layout &layout, logicitem_id_t logicitem_id)
    -> PlacedElement {
    return PlacedElement {
        .definition = to_logicitem_definition(layout, logicitem_id),
        .position = layout.logic_items().position(logicitem_id),
    };
}

auto to_display_state(wire_id_t wire_id) -> display_state_t {
    if (!wire_id) [[unlikely]] {
        throw std::runtime_error("invalid wire id has no display state");
    }
    if (wire_id == temporary_wire_id) {
        return display_state_t::temporary;
    }
    if (wire_id == colliding_wire_id) {
        return display_state_t::colliding;
    }
    return display_state_t::normal;
}

}  // namespace logicsim
