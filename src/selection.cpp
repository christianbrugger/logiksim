#include "selection.h"

#include "algorithm/merged_for_each.h"
#include "allocated_size/ankerl_unordered_dense.h"
#include "format/container.h"
#include "format/std_type.h"
#include "geometry/part_selections.h"
#include "geometry/rect.h"
#include "layout.h"
#include "layout_info.h"
#include "layout_message.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/point_fine.h"
#include "vocabulary/rect_fine.h"

namespace logicsim {

auto has_logic_items(const Selection &selection) -> bool {
    return !selection.selected_logic_items().empty();
}

auto get_lines(const Selection &selection, const Layout &layout)
    -> std::vector<ordered_line_t> {
    auto result = std::vector<ordered_line_t> {};

    for (const auto &entry : selection.selected_segments()) {
        const auto line = get_line(layout, entry.first);

        for (const auto part : entry.second) {
            result.push_back(to_line(line, part));
        }
    }

    return result;
}

auto anything_colliding(const Selection &selection, const Layout &layout) -> bool {
    const auto logicitem_colliding = [&](const logicitem_id_t &logicitem_id) {
        return layout.logic_items().display_state(logicitem_id) ==
               display_state_t::colliding;
    };
    const auto wire_colliding = [&](const Selection::segment_pair_t &pair) {
        return pair.first.wire_id == colliding_wire_id;
    };

    return std::ranges::any_of(selection.selected_segments(), wire_colliding) ||
           std::ranges::any_of(selection.selected_logic_items(), logicitem_colliding);
}

auto anything_temporary(const Selection &selection, const Layout &layout) -> bool {
    const auto logicitem_temporary = [&](const logicitem_id_t &logicitem_id) {
        return layout.logic_items().display_state(logicitem_id) ==
               display_state_t::temporary;
    };
    const auto wire_temporary = [&](const Selection::segment_pair_t &pair) {
        return pair.first.wire_id == temporary_wire_id;
    };

    return std::ranges::any_of(selection.selected_segments(), wire_temporary) ||
           std::ranges::any_of(selection.selected_logic_items(), logicitem_temporary);
}

auto anything_valid(const Selection &selection, const Layout &layout) -> bool {
    const auto logicitem_valid = [&](const logicitem_id_t &logicitem_id) {
        return layout.logic_items().display_state(logicitem_id) == display_state_t::valid;
    };
    const auto wire_valid = [&](const Selection::segment_pair_t &pair) {
        const auto &valid_parts = layout.wires()
                                      .segment_tree(pair.first.wire_id)
                                      .valid_parts(pair.first.segment_index);

        return a_overlaps_any_of_b(pair.second, valid_parts);
    };

    return std::ranges::any_of(selection.selected_segments(), wire_valid) ||
           std::ranges::any_of(selection.selected_logic_items(), logicitem_valid);
}

auto display_states(const Selection &selection, const Layout &layout) -> DisplayStateMap {
    auto result = DisplayStateMap {};

    // logic items
    for (const auto &logicitem_id : selection.selected_logic_items()) {
        result.at(layout.logic_items().display_state(logicitem_id)) = true;
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
            const auto &valid_parts = layout.wires()
                                          .segment_tree(pair.first.wire_id)
                                          .valid_parts(pair.first.segment_index);

            merged_for_each(pair.second, valid_parts,
                            [&](const part_t &a, const part_t &b) {
                                if (a_overlaps_any_of_b(a, b)) {
                                    result.at(display_state_t::valid) = true;
                                }
                                if (!a_inside_b(a, b)) {
                                    result.at(display_state_t::normal) = true;
                                }
                            });

            if (!pair.second.empty() && valid_parts.empty()) {
                result.at(display_state_t::normal) = true;
            }
        }
    }

    return result;
}

auto is_selected(const Selection &selection, const Layout &layout, segment_t segment,
                 point_fine_t point) -> bool {
    const auto full_line = get_line(layout, segment);

    for (const auto part : selection.selected_segments(segment)) {
        const auto line = to_line(full_line, part);
        const auto rect = element_selection_rect(line);

        if (is_colliding(point, rect)) {
            return true;
        }
    }

    return false;
}

//
// Selection
//

auto Selection::swap(Selection &other) noexcept -> void {
    using std::swap;

    selected_logicitems_.swap(other.selected_logicitems_);
    selected_segments_.swap(other.selected_segments_);
}

auto Selection::clear() -> void {
    selected_logicitems_.clear();
    selected_segments_.clear();
}

auto Selection::allocated_size() const -> std::size_t {
    return get_allocated_size(selected_logicitems_) +
           get_allocated_size(selected_segments_);
}

auto Selection::format() const -> std::string {
    return fmt::format(
        "Slection(\n"
        "  logic_items = {},\n"
        "  segments = {},\n"
        ")",
        selected_logicitems_.values(), selected_segments_.values());
}

auto Selection::format_info() const -> std::string {
    return fmt::format("Slection({} logic items, {} segments)",
                       selected_logicitems_.size(), selected_segments_.size());
}

auto Selection::empty() const noexcept -> bool {
    return selected_logicitems_.empty() && selected_segments_.empty();
}

auto Selection::add(logicitem_id_t logicitem_id) -> void {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("added element_id needs to be valid");
    }

    selected_logicitems_.insert(logicitem_id);
}

auto Selection::remove_logicitem(logicitem_id_t logicitem_id) -> void {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("removed logicitem_id needs to be valid");
    }

    selected_logicitems_.erase(logicitem_id);
}

auto Selection::toggle_logicitem(logicitem_id_t logicitem_id) -> void {
    if (!logicitem_id) [[unlikely]] {
        throw std::runtime_error("toggled logicitem_id needs to be valid");
    }

    if (is_selected(logicitem_id)) {
        remove_logicitem(logicitem_id);
    } else {
        add(logicitem_id);
    }
}

auto Selection::add_segment(segment_part_t segment_part) -> void {
    const auto it = selected_segments_.find(segment_part.segment);
    if (it == selected_segments_.end()) {
        // insert new list
        const auto value = detail::selection::map_value_t {segment_part.part};
        bool inserted = selected_segments_.emplace(segment_part.segment, value).second;
        if (!inserted) [[unlikely]] {
            throw std::runtime_error("unable to insert value");
        }
        return;
    }

    auto &entries = it->second;
    if (entries.size() == 0) [[unlikely]] {
        throw std::runtime_error("found segment selection with zero selection entries");
    }

    entries.add_part(segment_part.part);
}

auto Selection::remove_segment(segment_part_t segment_part) -> void {
    const auto it = selected_segments_.find(segment_part.segment);
    if (it == selected_segments_.end()) {
        return;
    }

    auto &entries = it->second;
    if (entries.size() == 0) [[unlikely]] {
        throw std::runtime_error("found segment selection with zero selection entries");
    }

    entries.remove_part(segment_part.part);

    if (entries.empty()) {
        if (!selected_segments_.erase(segment_part.segment)) {
            throw std::runtime_error("unable to delete key");
        }
    }
}

auto Selection::set_selection(segment_t segment, PartSelection &&parts) -> void {
    const auto it = selected_segments_.find(segment);

    if (parts.empty()) {
        if (it != selected_segments_.end()) {
            selected_segments_.erase(it);
        }
        return;
    }

    if (it != selected_segments_.end()) {
        using std::swap;
        swap(it->second, parts);
        return;
    }

    selected_segments_.emplace(segment, std::move(parts));
}

auto Selection::is_selected(logicitem_id_t logicitem_id) const -> bool {
    return selected_logicitems_.contains(logicitem_id);
}

auto Selection::is_selected(segment_t segment) const -> bool {
    return selected_segments_.contains(segment);
}

auto Selection::selected_logic_items() const -> std::span<const logicitem_id_t> {
    return selected_logicitems_.values();
}

auto Selection::selected_segments() const -> std::span<const segment_pair_t> {
    return selected_segments_.values();
}

auto Selection::selected_segments(segment_t segment) const -> const PartSelection & {
    // constexpr static auto selection = part_selection::part_vector_t {};

    // TODO !!! make constexpr
    const static auto empty_selection = PartSelection {};

    const auto it = selected_segments_.find(segment);
    if (it == selected_segments_.end()) {
        return empty_selection;
    }

    auto &entries = it->second;
    if (entries.size() == 0) [[unlikely]] {
        throw std::runtime_error("found segment selection with zero selection entries");
    }

    return it->second;
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

auto Selection::handle(const editable_circuit::info_message::LogicItemDeleted &message)
    -> void {
    remove_logicitem(message.logicitem_id);
}

auto Selection::handle(const editable_circuit::info_message::LogicItemIdUpdated &message)
    -> void {
    const auto found = selected_logicitems_.erase(message.old_logicitem_id);
    if (found) {
        const auto inserted =
            selected_logicitems_.insert(message.new_logicitem_id).second;

        if (!inserted) [[unlikely]] {
            throw std::runtime_error("element already existed");
        }
    }
}

auto Selection::handle(const editable_circuit::info_message::SegmentIdUpdated &message)
    -> void {
    const auto it = selected_segments_.find(message.old_segment);

    if (it != selected_segments_.end()) {
        auto parts = detail::selection::map_value_t {std::move(it->second)};
        selected_segments_.erase(it);

        const auto inserted =
            selected_segments_.emplace(message.new_segment, std::move(parts)).second;

        if (!inserted) [[unlikely]] {
            throw std::runtime_error("line segment already existed");
        }
    }
}

auto handle_move_different_segment(
    detail::selection::segment_map_t &map,
    editable_circuit::info_message::SegmentPartMoved message) {
    using namespace detail::selection;
    if (message.segment_part_source.segment == message.segment_part_destination.segment)
        [[unlikely]] {
        throw std::runtime_error("source and destination need to be different");
    }

    // find source entries
    const auto it_source = map.find(message.segment_part_source.segment);
    if (it_source == map.end()) {
        // nothing to copy
        return;
    }
    auto &source_entries = it_source->second;

    // find destination entries
    auto destination_entries = [&]() {
        const auto it_dest = map.find(message.segment_part_destination.segment);
        return it_dest != map.end() ? it_dest->second : map_value_t {};
    }();

    // move
    move_parts({
        .destination = destination_entries,
        .source = source_entries,
        .copy_definition =
            part_copy_definition_t {
                .destination = message.segment_part_destination.part,
                .source = message.segment_part_source.part,
            },
    });

    // delete source
    if (source_entries.empty()) {
        map.erase(message.segment_part_source.segment);
    }

    // add destination
    if (!destination_entries.empty()) {
        map.insert_or_assign(message.segment_part_destination.segment,
                             std::move(destination_entries));
    }
}

auto handle_move_same_segment(detail::selection::segment_map_t &map,
                              editable_circuit::info_message::SegmentPartMoved message) {
    if (message.segment_part_source.segment != message.segment_part_destination.segment)
        [[unlikely]] {
        throw std::runtime_error("source and destination need to the same");
    }

    // find entries
    const auto it = map.find(message.segment_part_source.segment);
    if (it == map.end()) {
        // nothing to copy
        return;
    }
    auto &entries = it->second;

    move_parts(entries, part_copy_definition_t {
                            .destination = message.segment_part_destination.part,
                            .source = message.segment_part_source.part,
                        });

    if (entries.empty()) [[unlikely]] {
        throw std::runtime_error("result should never be empty");
    }
}

auto Selection::handle(const editable_circuit::info_message::SegmentPartMoved &message)
    -> void {
    if (message.segment_part_source.segment == message.segment_part_destination.segment) {
        handle_move_same_segment(selected_segments_, message);
    } else {
        handle_move_different_segment(selected_segments_, message);
    }
}

auto Selection::handle(const editable_circuit::info_message::SegmentPartDeleted &message)
    -> void {
    remove_segment(message.segment_part);
}

auto Selection::submit(const editable_circuit::InfoMessage &message) -> void {
    using namespace editable_circuit::info_message;

    // logic item
    if (const auto pointer = std::get_if<LogicItemDeleted>(&message)) {
        handle(*pointer);
        return;
    }
    if (const auto pointer = std::get_if<LogicItemIdUpdated>(&message)) {
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

//
// validation
//

namespace {

using namespace detail::selection;

auto check_and_remove_segments(detail::selection::segment_map_t &segment_map,
                               const wire_id_t wire_id, const SegmentTree &segment_tree)
    -> void {
    for (const auto segment_index : segment_tree.indices()) {
        const auto key = segment_t {wire_id, segment_index};
        const auto it = segment_map.find(key);

        if (it != segment_map.end()) {
            const auto line = segment_tree.line(segment_index);

            if (it->second.max_offset() > to_part(line).end) [[unlikely]] {
                throw std::runtime_error("parts are not part of line");
            }

            segment_map.erase(it);
        }
    }
}

}  // namespace

auto Selection::validate(const Layout &layout) const -> void {
    auto logicitems_set = detail::selection::logicitems_set_t {selected_logicitems_};
    auto segment_map = detail::selection::segment_map_t {selected_segments_};

    // logic items
    for (const auto logicitem_id : logicitem_ids(layout)) {
        logicitems_set.erase(logicitem_id);
    }
    if (!logicitems_set.empty()) [[unlikely]] {
        throw std::runtime_error("selection contains elements that don't exist anymore");
    }

    // segments
    for (const auto wire_id : wire_ids(layout)) {
        check_and_remove_segments(segment_map, wire_id,
                                  layout.wires().segment_tree(wire_id));
    }
    if (!segment_map.empty()) [[unlikely]] {
        throw std::runtime_error("selection contains segments that don't exist anymore");
    }
}

// Section

auto add_segment(Selection &selection, segment_t segment, const Layout &layout) -> void {
    const auto part = to_part(get_line(layout, segment));
    selection.add_segment(segment_part_t {segment, part});
}

auto add_segment_tree(Selection &selection, wire_id_t wire_id, const Layout &layout)
    -> void {
    const auto &tree = layout.wires().segment_tree(wire_id);
    for (const auto &segment_index : tree.indices()) {
        add_segment(selection, segment_t {wire_id, segment_index}, layout);
    }
}

auto remove_segment(Selection &selection, segment_t segment, const Layout &layout)
    -> void {
    const auto part = to_part(get_line(layout, segment));
    selection.remove_segment(segment_part_t {segment, part});
}

auto remove_segment_tree(Selection &selection, wire_id_t wire_id, const Layout &layout)
    -> void {
    const auto &tree = layout.wires().segment_tree(wire_id);
    for (const auto &segment_index : tree.indices()) {
        remove_segment(selection, segment_t {wire_id, segment_index}, layout);
    }
}

auto add_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                      point_fine_t point) -> void {
    const auto full_line = get_line(layout, segment);
    const auto &parts = selection.selected_segments(segment);

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
    const auto &parts = selection.selected_segments(segment);

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

}  // namespace logicsim