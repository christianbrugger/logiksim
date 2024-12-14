#include "core/selection.h"

#include "core/algorithm/merged_for_each.h"
#include "core/allocated_size/ankerl_unordered_dense.h"
#include "core/format/container.h"
#include "core/format/std_type.h"
#include "core/geometry/display_state_map.h"
#include "core/geometry/part_selections.h"
#include "core/geometry/rect.h"
#include "core/layout.h"
#include "core/layout_info.h"
#include "core/layout_message.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/point_fine.h"
#include "core/vocabulary/rect_fine.h"

#include <algorithm>
#include <cassert>

namespace logicsim {

//
// Selection
//

Selection::Selection(std::span<const logicitem_id_t> logicitems,
                     std::span<const decoration_id_t> decorations) {
    for (const auto logicitem : logicitems) {
        add_logicitem(logicitem);
    }
    for (const auto decoration : decorations) {
        add_decoration(decoration);
    }
}

Selection::Selection(logicitems_set_t &&logicitems, decorations_set_t &&decorations,
                     segment_map_t &&segments)
    : selected_logicitems_ {std::move(logicitems)},
      selected_decorations_ {std::move(decorations)},
      selected_segments_ {std::move(segments)} {}

auto Selection::format() const -> std::string {
    Expects(class_invariant_holds());

    return fmt::format(
        "Slection(\n"
        "  logicitems = {},\n"
        "  segments = {},\n"
        "  decorations = {},\n"
        ")",
        selected_logicitems_.values(), selected_segments_.values(),
        selected_decorations_.values());
}

auto Selection::format_info(bool as_selection) const -> std::string {
    Expects(class_invariant_holds());

    if (as_selection) {
        return fmt::format("Slection({} logic items, {} segments, {} decorations)",
                           selected_logicitems_.size(), selected_segments_.size(),
                           selected_decorations_.size());
    }
    return fmt::format("{} logic items, {} segments and {} decorations",
                       selected_logicitems_.size(), selected_segments_.size(),
                       selected_decorations_.size());
}

auto Selection::empty() const -> bool {
    Expects(class_invariant_holds());

    return selected_logicitems_.empty() && selected_segments_.empty() &&
           selected_decorations_.empty();
}

auto Selection::size() const -> std::size_t {
    return selected_logicitems_.size() + selected_segments_.size() +
           selected_decorations_.size();
}

auto Selection::clear() -> void {
    Expects(class_invariant_holds());

    selected_logicitems_.clear();
    selected_segments_.clear();
    selected_decorations_.clear();

    Ensures(class_invariant_holds());
}

auto Selection::allocated_size() const -> std::size_t {
    Expects(class_invariant_holds());

    return get_allocated_size(selected_logicitems_) +
           get_allocated_size(selected_segments_) +
           get_allocated_size(selected_decorations_);
}

auto Selection::add_logicitem(logicitem_id_t logicitem_id) -> void {
    Expects(class_invariant_holds());

    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("added logicitem_id needs to be valid");
    }

    selected_logicitems_.insert(logicitem_id);

    Ensures(class_invariant_holds());
}

auto Selection::remove_logicitem(logicitem_id_t logicitem_id) -> void {
    Expects(class_invariant_holds());

    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("removed logicitem_id needs to be valid");
    }

    selected_logicitems_.erase(logicitem_id);

    Ensures(class_invariant_holds());
}

auto Selection::toggle_logicitem(logicitem_id_t logicitem_id) -> void {
    Expects(class_invariant_holds());

    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("toggled logicitem_id needs to be valid");
    }

    if (is_selected(logicitem_id)) {
        remove_logicitem(logicitem_id);
    } else {
        add_logicitem(logicitem_id);
    }

    Ensures(class_invariant_holds());
}

auto Selection::add_decoration(decoration_id_t decoration_id) -> void {
    Expects(class_invariant_holds());

    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("added decoration_id needs to be valid");
    }

    selected_decorations_.insert(decoration_id);

    Ensures(class_invariant_holds());
}

auto Selection::remove_decoration(decoration_id_t decoration_id) -> void {
    Expects(class_invariant_holds());

    if (!decoration_id) [[unlikely]] {
        throw std::runtime_error("removed decoration_id needs to be valid");
    }

    selected_decorations_.erase(decoration_id);

    Ensures(class_invariant_holds());
}

auto Selection::add_segment(segment_part_t segment_part) -> void {
    Expects(class_invariant_holds());

    const auto it = selected_segments_.find(segment_part.segment);

    if (it == selected_segments_.end()) {
        // not found
        const auto value = selection::map_value_t {segment_part.part};
        bool inserted = selected_segments_.emplace(segment_part.segment, value).second;
        Expects(inserted);
    } else {
        // found
        auto &entries = it->second;
        Expects(!entries.empty());

        entries.add_part(segment_part.part);
    }

    Ensures(class_invariant_holds());
}

auto Selection::remove_segment(segment_part_t segment_part) -> void {
    Expects(class_invariant_holds());

    const auto it = selected_segments_.find(segment_part.segment);

    if (it != selected_segments_.end()) {
        auto &entries = it->second;
        Expects(!entries.empty());

        entries.remove_part(segment_part.part);

        if (entries.empty()) {
            Expects(selected_segments_.erase(segment_part.segment));
        }
    }

    Ensures(class_invariant_holds());
}

auto Selection::set_selection(segment_t segment, PartSelection &&parts) -> void {
    Expects(class_invariant_holds());

    const auto it = selected_segments_.find(segment);

    if (it == selected_segments_.end()) {
        // not found
        Expects(selected_segments_.emplace(segment, std::move(parts)).second);
    } else {
        // found
        if (parts.empty()) {
            selected_segments_.erase(it);
        } else {
            using std::swap;
            swap(it->second, parts);
        }
    }

    Ensures(class_invariant_holds());
}

auto Selection::is_selected(logicitem_id_t logicitem_id) const -> bool {
    Expects(class_invariant_holds());

    return selected_logicitems_.contains(logicitem_id);
}

auto Selection::is_selected(decoration_id_t decoration_id) const -> bool {
    Expects(class_invariant_holds());

    return selected_decorations_.contains(decoration_id);
}

auto Selection::is_selected(segment_t segment) const -> bool {
    Expects(class_invariant_holds());

    return selected_segments_.contains(segment);
}

auto Selection::selected_logicitems() const -> std::span<const logicitem_id_t> {
    Expects(class_invariant_holds());

    return selected_logicitems_.values();
}

auto Selection::selected_decorations() const -> std::span<const decoration_id_t> {
    Expects(class_invariant_holds());

    return selected_decorations_.values();
}

auto Selection::selected_segments() const -> std::span<const segment_pair_t> {
    Expects(class_invariant_holds());

    return selected_segments_.values();
}

auto Selection::selected_segments(segment_t segment) const -> const PartSelection & {
    Expects(class_invariant_holds());

    const static thread_local auto empty_selection = PartSelection {};

    const auto it = selected_segments_.find(segment);
    if (it == selected_segments_.end()) {
        return empty_selection;
    }

    auto &entries = it->second;
    Expects(!entries.empty());

    return it->second;
}

//
// Handle Methods
//

auto Selection::handle(const info_message::LogicItemDeleted &message) -> void {
    Expects(class_invariant_holds());

    remove_logicitem(message.logicitem_id);

    Ensures(class_invariant_holds());
}

auto Selection::handle(const info_message::LogicItemIdUpdated &message) -> void {
    Expects(class_invariant_holds());

    const auto found = selected_logicitems_.erase(message.old_logicitem_id);
    if (found != 0) {
        const auto added = selected_logicitems_.insert(message.new_logicitem_id).second;
        Expects(added);
    }

    Ensures(class_invariant_holds());
}

auto Selection::handle(const info_message::DecorationDeleted &message) -> void {
    Expects(class_invariant_holds());

    remove_decoration(message.decoration_id);

    Ensures(class_invariant_holds());
}

auto Selection::handle(const info_message::DecorationIdUpdated &message) -> void {
    Expects(class_invariant_holds());

    const auto found = selected_decorations_.erase(message.old_decoration_id);
    if (found != 0) {
        const auto added = selected_decorations_.insert(message.new_decoration_id).second;
        Expects(added);
    }

    Ensures(class_invariant_holds());
}

auto Selection::handle(const info_message::SegmentIdUpdated &message) -> void {
    Expects(class_invariant_holds());

    const auto it = selected_segments_.find(message.old_segment);

    if (it != selected_segments_.end()) {
        auto parts = selection::map_value_t {std::move(it->second)};
        selected_segments_.erase(it);

        const auto added =
            selected_segments_.emplace(message.new_segment, std::move(parts)).second;
        Expects(added);
    }

    Ensures(class_invariant_holds());
}

namespace {

auto _handle_move_different_segment(selection::segment_map_t &map,
                                    info_message::SegmentPartMoved message) -> void {
    using namespace selection;

    if (message.source.segment == message.destination.segment) [[unlikely]] {
        throw std::runtime_error("source and destination need to be different");
    }

    // find source entries
    const auto it_source = map.find(message.source.segment);
    if (it_source == map.end()) {
        // nothing to copy
        return;
    }
    auto &source_entries = it_source->second;

    // find destination entries
    auto destination_entries = [&]() {
        const auto it_dest = map.find(message.destination.segment);
        return it_dest != map.end() ? it_dest->second : map_value_t {};
    }();

    // move
    move_parts({
        .destination = destination_entries,
        .source = source_entries,
        .copy_definition =
            part_copy_definition_t {
                .destination = message.destination.part,
                .source = message.source.part,
            },
    });

    // delete source
    if (source_entries.empty()) {
        map.erase(message.source.segment);
    }

    // add destination
    if (!destination_entries.empty()) {
        map.insert_or_assign(message.destination.segment, std::move(destination_entries));
    }
}

auto _handle_move_same_segment(selection::segment_map_t &map,
                               info_message::SegmentPartMoved message) -> void {
    if (message.source.segment != message.destination.segment) [[unlikely]] {
        throw std::runtime_error("source and destination need to the same");
    }

    // find entries
    const auto it = map.find(message.source.segment);
    if (it == map.end()) {
        // nothing to copy
        return;
    }
    auto &entries = it->second;

    move_parts(entries, part_copy_definition_t {
                            .destination = message.destination.part,
                            .source = message.source.part,
                        });

    Expects(!entries.empty());
}

}  // namespace

auto Selection::handle(const info_message::SegmentPartMoved &message) -> void {
    Expects(class_invariant_holds());

    if (message.source.segment == message.destination.segment) {
        _handle_move_same_segment(selected_segments_, message);
    } else {
        _handle_move_different_segment(selected_segments_, message);
    }

    Ensures(class_invariant_holds());
}

auto Selection::handle(const info_message::SegmentPartDeleted &message) -> void {
    Expects(class_invariant_holds());

    remove_segment(message.segment_part);

    Ensures(class_invariant_holds());
}

auto Selection::submit(const InfoMessage &message) -> void {
    using namespace info_message;

    // logic item
    if (const auto pointer = std::get_if<LogicItemDeleted>(&message)) {
        handle(*pointer);
        return;
    }
    if (const auto pointer = std::get_if<LogicItemIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }

    // decoration
    if (const auto pointer = std::get_if<DecorationDeleted>(&message)) {
        handle(*pointer);
        return;
    }
    if (const auto pointer = std::get_if<DecorationIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }

    // segments
    if (const auto pointer = std::get_if<SegmentIdUpdated>(&message)) {
        handle(*pointer);
        return;
    }
    if (const auto pointer = std::get_if<SegmentPartMoved>(&message)) {
        handle(*pointer);
        return;
    }
    if (const auto pointer = std::get_if<SegmentPartDeleted>(&message)) {
        handle(*pointer);
        return;
    }
}

auto Selection::class_invariant_holds() const -> bool {
    // logicitem ids are not null
    assert(std::ranges::all_of(selected_logicitems_, &logicitem_id_t::operator bool));
    // decorations ids are not null
    assert(std::ranges::all_of(selected_decorations_, &decoration_id_t::operator bool));
    // segment ids are not null
    assert(std::ranges::all_of(std::ranges::views::keys(selected_segments_),
                               &segment_t::operator bool));

    // part-selections are not empty
    assert(std::ranges::none_of(std::ranges::views::values(selected_segments_),
                                &PartSelection::empty));

    return true;
}

//
// Free Functions
//

auto is_valid_selection(const Selection &selection, const Layout &layout) -> bool {
    const auto logicitem_valid = [&](const logicitem_id_t &logicitem_id) -> bool {
        return is_id_valid(logicitem_id, layout);
    };

    const auto decoration_valid = [&](const decoration_id_t &decoration_id) -> bool {
        return is_id_valid(decoration_id, layout);
    };

    const auto segment_valid = [&](const Selection::segment_pair_t &entry) -> bool {
        const auto &[segment, parts] = entry;
        const auto segment_part =
            segment_part_t {segment, part_t {0, parts.max_offset()}};
        return is_segment_part_valid(segment_part, layout);
    };

    return std::ranges::all_of(selection.selected_logicitems(), logicitem_valid) &&
           std::ranges::all_of(selection.selected_decorations(), decoration_valid) &&
           std::ranges::all_of(selection.selected_segments(), segment_valid);
}

//
//
//

auto has_logicitems(const Selection &selection) -> bool {
    return !selection.selected_logicitems().empty();
}

auto has_decorations(const Selection &selection) -> bool {
    return !selection.selected_decorations().empty();
}

auto get_lines(const Selection &selection,
               const Layout &layout) -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};

    for (const auto &entry : selection.selected_segments()) {
        const auto line = get_line(layout, entry.first);

        for (const auto part : entry.second) {
            result.push_back(to_line(line, part));
        }
    }

    return result;
}

auto all_normal_display_state(const Selection &selection, const Layout &layout) -> bool {
    const auto logicitem_normal = [&](const logicitem_id_t &logicitem_id) {
        return layout.logicitems().display_state(logicitem_id) == display_state_t::normal;
    };
    const auto decoration_normal = [&](const decoration_id_t &decoration_id) {
        return layout.decorations().display_state(decoration_id) ==
               display_state_t::normal;
    };
    const auto wire_normal = [&](const Selection::segment_pair_t &pair) {
        return is_inserted(pair.first.wire_id) &&
               a_disjoint_b(pair.second, get_segment_valid_parts(layout, pair.first));
    };

    return std::ranges::all_of(selection.selected_logicitems(), logicitem_normal) &&
           std::ranges::all_of(selection.selected_decorations(), decoration_normal) &&
           std::ranges::all_of(selection.selected_segments(), wire_normal);
}

auto anything_colliding(const Selection &selection, const Layout &layout) -> bool {
    const auto logicitem_colliding = [&](const logicitem_id_t &logicitem_id) {
        return layout.logicitems().display_state(logicitem_id) ==
               display_state_t::colliding;
    };
    const auto decoration_colliding = [&](const decoration_id_t &decoration_id) {
        return layout.decorations().display_state(decoration_id) ==
               display_state_t::colliding;
    };
    const auto wire_colliding = [&](const Selection::segment_pair_t &pair) {
        return pair.first.wire_id == colliding_wire_id;
    };

    return std::ranges::any_of(selection.selected_segments(), wire_colliding) ||
           std::ranges::any_of(selection.selected_decorations(), decoration_colliding) ||
           std::ranges::any_of(selection.selected_logicitems(), logicitem_colliding);
}

auto anything_temporary(const Selection &selection, const Layout &layout) -> bool {
    const auto logicitem_temporary = [&](const logicitem_id_t &logicitem_id) {
        return layout.logicitems().display_state(logicitem_id) ==
               display_state_t::temporary;
    };
    const auto decoration_temporary = [&](const decoration_id_t &decoration_id) {
        return layout.decorations().display_state(decoration_id) ==
               display_state_t::temporary;
    };
    const auto wire_temporary = [&](const Selection::segment_pair_t &pair) {
        return pair.first.wire_id == temporary_wire_id;
    };

    return std::ranges::any_of(selection.selected_segments(), wire_temporary) ||
           std::ranges::any_of(selection.selected_decorations(), decoration_temporary) ||
           std::ranges::any_of(selection.selected_logicitems(), logicitem_temporary);
}

auto anything_valid(const Selection &selection, const Layout &layout) -> bool {
    const auto logicitem_valid = [&](const logicitem_id_t &logicitem_id) {
        return layout.logicitems().display_state(logicitem_id) == display_state_t::valid;
    };
    const auto decoration_valid = [&](const decoration_id_t &decoration_id) {
        return layout.decorations().display_state(decoration_id) ==
               display_state_t::valid;
    };
    const auto wire_valid = [&](const Selection::segment_pair_t &pair) {
        const auto &valid_parts = layout.wires()
                                      .segment_tree(pair.first.wire_id)
                                      .valid_parts(pair.first.segment_index);

        return a_overlaps_any_of_b(pair.second, valid_parts);
    };

    return std::ranges::any_of(selection.selected_segments(), wire_valid) ||
           std::ranges::any_of(selection.selected_decorations(), decoration_valid) ||
           std::ranges::any_of(selection.selected_logicitems(), logicitem_valid);
}

auto display_states(const Selection &selection, const Layout &layout) -> DisplayStateMap {
    auto result = DisplayStateMap {};

    // logic items
    for (const auto &logicitem_id : selection.selected_logicitems()) {
        result.at(layout.logicitems().display_state(logicitem_id)) = true;
    }

    // decorations
    for (const auto &decoration_id : selection.selected_decorations()) {
        result.at(layout.decorations().display_state(decoration_id)) = true;
    }

    // wires
    for (const Selection::segment_pair_t &pair : selection.selected_segments()) {
        if (pair.first.wire_id == temporary_wire_id) {
            result.at(display_state_t::temporary) = true;
        }

        else if (pair.first.wire_id == colliding_wire_id) {
            result.at(display_state_t::colliding) = true;
        }

        else if (!result.at(display_state_t::valid) ||
                 !result.at(display_state_t::normal)) {
            const auto &segment_tree = layout.wires().segment_tree(pair.first.wire_id);
            const auto full_part = segment_tree.part(pair.first.segment_index);

            const auto &valid_parts = segment_tree.valid_parts(pair.first.segment_index);
            const auto &selected_parts = pair.second;

            iter_overlapping_parts(full_part, selected_parts, valid_parts,
                                   [&](part_t, part_t, bool valid) {
                                       if (valid) {
                                           result.at(display_state_t::valid) = true;
                                       } else {
                                           result.at(display_state_t::normal) = true;
                                       }
                                   });
        }
    }

    Ensures(selection.empty() == (count_values(result) == 0));
    return result;
}

auto is_selected(const Selection &selection, const Layout &layout, segment_t segment,
                 point_fine_t point) -> bool {
    const auto is_part_selected =
        [point, full_line = get_line(layout, segment)](const part_t &part) {
            const auto line = to_line(full_line, part);
            const auto rect = element_selection_rect(line);
            return is_colliding(point, rect);
        };

    return std::ranges::any_of(selection.selected_segments(segment), is_part_selected);
}

auto is_full_segment(const segment_t segment, const PartSelection &parts,
                     const Layout &layout) -> bool {
    return parts.size() == 1 && parts.front() == to_part(get_line(layout, segment));
}

auto is_full_segment(const selection::segment_pair_t &pair,
                     const Layout &layout) -> bool {
    return is_full_segment(pair.first, pair.second, layout);
}

auto is_full_segment(const Selection &selection, segment_t segment,
                     const Layout &layout) -> bool {
    return is_full_segment(segment, selection.selected_segments(segment), layout);
}

//
//
//

auto add_segment(Selection &selection, segment_t segment, const Layout &layout) -> void {
    const auto part = to_part(get_line(layout, segment));
    selection.add_segment(segment_part_t {segment, part});
}

auto add_segment_tree(Selection &selection, wire_id_t wire_id,
                      const Layout &layout) -> void {
    const auto &tree = layout.wires().segment_tree(wire_id);
    for (const auto &segment_index : tree.indices()) {
        selection.add_segment(segment_part_t {segment_t {wire_id, segment_index},
                                              tree.part(segment_index)});
    }
}

auto remove_segment(Selection &selection, segment_t segment,
                    const Layout &layout) -> void {
    const auto part = to_part(get_line(layout, segment));
    selection.remove_segment(segment_part_t {segment, part});
}

auto remove_segment_tree(Selection &selection, wire_id_t wire_id,
                         const Layout &layout) -> void {
    const auto &tree = layout.wires().segment_tree(wire_id);
    for (const auto &segment_index : tree.indices()) {
        selection.remove_segment(segment_part_t {segment_t {wire_id, segment_index},
                                                 tree.part(segment_index)});
    }
}

auto add_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                      point_fine_t point) -> void {
    const auto full_line = get_line(layout, segment);
    const auto parts = PartSelection {selection.selected_segments(segment)};

    iter_parts(to_part(full_line), parts, [&](part_t part, bool) {
        const auto line = to_line(full_line, part);
        const auto rect = element_selection_rect(line);
        if (is_colliding(point, rect)) {
            selection.add_segment(segment_part_t {segment, part});
        }
    });
}

auto remove_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                         point_fine_t point) -> void {
    const auto full_line = get_line(layout, segment);

    for (auto part : selection.selected_segments(segment)) {
        const auto line = to_line(full_line, part);
        const auto rect = element_selection_rect(line);
        if (is_colliding(point, rect)) {
            selection.remove_segment(segment_part_t {segment, part});
        }
    }
}

auto toggle_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                         point_fine_t point) -> void {
    const auto full_line = get_line(layout, segment);
    const auto parts = PartSelection {selection.selected_segments(segment)};

    iter_parts(to_part(full_line), parts, [&](part_t part, bool selected) {
        const auto line = to_line(full_line, part);
        const auto rect = element_selection_rect(line);
        if (is_colliding(point, rect)) {
            if (selected) {
                selection.remove_segment(segment_part_t {segment, part});
            } else {
                selection.add_segment(segment_part_t {segment, part});
            }
        }
    });
}

auto select_all(const Layout &layout) -> Selection {
    auto result = Selection {};

    for (const auto logicitem_id : logicitem_ids(layout)) {
        result.add_logicitem(logicitem_id);
    }

    for (const auto decoration_id : decoration_ids(layout)) {
        result.add_decoration(decoration_id);
    }

    for (const auto wire_id : wire_ids(layout)) {
        add_segment_tree(result, wire_id, layout);
    }

    return result;
}

auto is_all_selected(const Selection &selection, const Layout &layout) -> bool {
    const auto is_item_selected = [&](const auto &item) -> bool {
        return selection.is_selected(item);
    };
    const auto segment_fully_selected = [&](const auto &segment) -> bool {
        return is_full_segment(selection, segment, layout);
    };
    const auto wire_selected = [&](const wire_id_t &wire_id) -> bool {
        return std::ranges::all_of(layout.wires().segment_tree(wire_id).indices(wire_id),
                                   segment_fully_selected);
    };

    return layout.logicitems().size() == selection.selected_logicitems().size() &&
           layout.decorations().size() == selection.selected_decorations().size() &&
           get_segment_count(layout) == selection.selected_segments().size() &&
           std::ranges::all_of(logicitem_ids(layout), is_item_selected) &&
           std::ranges::all_of(decoration_ids(layout), is_item_selected) &&
           std::ranges::all_of(wire_ids(layout), wire_selected);
}

auto get_single_logicitem(const Selection &selection) -> logicitem_id_t {
    if (selection.selected_logicitems().size() != 1 || selection.size() != 1) {
        return null_logicitem_id;
    }
    return selection.selected_logicitems().front();
}

auto get_single_decoration(const Selection &selection) -> decoration_id_t {
    if (selection.selected_decorations().size() != 1 || selection.size() != 1) {
        return null_decoration_id;
    }
    return selection.selected_decorations().front();
}

auto get_single_segment(const Selection &selection)
    -> std::optional<selection::segment_pair_t> {
    if (selection.selected_segments().size() != 1 || selection.size() != 1) {
        return std::nullopt;
    }
    return selection.selected_segments().front();
}

}  // namespace logicsim
