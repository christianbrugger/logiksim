
#include "editable_circuit/selection.h"

#include "editable_circuit/message.h"
#include "exception.h"
#include "format/container.h"
#include "format/std_type.h"
#include "geometry/rect.h"
#include "layout.h"
#include "layout_info.h"
#include "geometry/part_list.h"
#include "geometry/part_list_copying.h"

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
    for (const auto element_id : selection.selected_logic_items()) {
        if (layout.display_state(element_id) == display_state_t::colliding) {
            return true;
        }
    }

    for (const auto &[segment, _] : selection.selected_segments()) {
        if (layout.display_state(segment.element_id) == display_state_t::colliding) {
            return true;
        }
    }

    return false;
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

auto Selection::add_logicitem(element_id_t element_id) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("added element_id needs to be valid");
    }

    selected_logicitems_.insert(element_id);
}

auto Selection::remove_logicitem(element_id_t element_id) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("removed element_id needs to be valid");
    }

    selected_logicitems_.erase(element_id);
}

auto Selection::toggle_logicitem(element_id_t element_id) -> void {
    if (!element_id) [[unlikely]] {
        throw_exception("toggled element_id needs to be valid");
    }

    if (is_selected(element_id)) {
        remove_logicitem(element_id);
    } else {
        add_logicitem(element_id);
    }
}

auto Selection::add_segment(segment_part_t segment_part) -> void {
    const auto it = selected_segments_.find(segment_part.segment);
    if (it == selected_segments_.end()) {
        // insert new list
        const auto value = detail::selection::map_value_t {segment_part.part};
        bool inserted = selected_segments_.emplace(segment_part.segment, value).second;
        if (!inserted) [[unlikely]] {
            throw_exception("unable to insert value");
        }
        return;
    }

    auto &entries = it->second;
    if (entries.size() == 0) [[unlikely]] {
        throw_exception("found segment selection with zero selection entries");
    }

    add_part(entries, segment_part.part);
}

auto Selection::remove_segment(segment_part_t segment_part) -> void {
    const auto it = selected_segments_.find(segment_part.segment);
    if (it == selected_segments_.end()) {
        return;
    }

    auto &entries = it->second;
    if (entries.size() == 0) [[unlikely]] {
        throw_exception("found segment selection with zero selection entries");
    }

    remove_part(entries, segment_part.part);

    if (entries.empty()) {
        if (!selected_segments_.erase(segment_part.segment)) {
            throw_exception("unable to delete key");
        }
    }
}

auto Selection::set_selection(segment_t segment, part_vector_t &&parts) -> void {
    const auto it = selected_segments_.find(segment);

    if (parts.empty()) {
        if (it != selected_segments_.end()) {
            selected_segments_.erase(it);
        }
        return;
    }

    if (it != selected_segments_.end()) {
        it->second.swap(parts);
        return;
    }

    selected_segments_.emplace(segment, std::move(parts));
}

auto Selection::is_selected(element_id_t element_id) const -> bool {
    return selected_logicitems_.contains(element_id);
}

auto Selection::is_selected(segment_t segment) const -> bool {
    return selected_segments_.contains(segment);
}

auto Selection::selected_logic_items() const -> std::span<const element_id_t> {
    return selected_logicitems_.values();
}

auto Selection::selected_segments() const -> std::span<const segment_pair_t> {
    return selected_segments_.values();
}

auto Selection::selected_segments(segment_t segment) const -> std::span<const part_t> {
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
    remove_logicitem(message.element_id);
}

auto Selection::handle(const editable_circuit::info_message::LogicItemIdUpdated &message)
    -> void {
    const auto found = selected_logicitems_.erase(message.old_element_id);
    if (found) {
        const auto inserted = selected_logicitems_.insert(message.new_element_id).second;

        if (!inserted) [[unlikely]] {
            throw_exception("element already existed");
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
            throw_exception("line segment already existed");
        }
    }
}

auto handle_move_different_segment(
    detail::selection::segment_map_t &map,
    editable_circuit::info_message::SegmentPartMoved message) {
    using namespace detail::selection;
    if (message.segment_part_source.segment == message.segment_part_destination.segment)
        [[unlikely]] {
        throw_exception("source and destination need to be different");
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
    const auto parts = part_copy_definition_t {
        .destination = message.segment_part_destination.part,
        .source = message.segment_part_source.part,
    };
    move_parts(source_entries, destination_entries, parts);

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
        throw_exception("source and destination need to the same");
    }

    // find entries
    const auto it = map.find(message.segment_part_source.segment);
    if (it == map.end()) {
        // nothing to copy
        return;
    }
    auto &entries = it->second;

    // move to new copy
    const auto parts = part_copy_definition_t {
        .destination = message.segment_part_destination.part,
        .source = message.segment_part_source.part,
    };
    auto result = entries;
    remove_part(result, parts.source);
    copy_parts(entries, result, parts);

    // insert result
    if (result.empty()) [[unlikely]] {
        throw_exception("result should never be empty");
    }
    map.insert_or_assign(message.segment_part_destination.segment, std::move(result));
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
                               const element_id_t element_id,
                               const SegmentTree &segment_tree) -> void {
    for (const auto segment_index : segment_tree.indices()) {
        const auto key = segment_t {element_id, segment_index};
        const auto it = segment_map.find(key);

        if (it != segment_map.end()) {
            const auto line = segment_tree.segment_line(segment_index);
            sort_and_validate_segment_parts(it->second, line);
            segment_map.erase(it);
        }
    }
}

}  // namespace

auto Selection::validate(const Layout &layout) const -> void {
    auto logicitems_set = detail::selection::logicitems_set_t {selected_logicitems_};
    auto segment_map = detail::selection::segment_map_t {selected_segments_};

    // logic items
    for (const auto element : layout.elements()) {
        if (element.is_logic_item()) {
            logicitems_set.erase(element.element_id());
        }
    }
    if (!logicitems_set.empty()) [[unlikely]] {
        throw_exception("selection contains elements that don't exist anymore");
    }

    // segments
    for (const auto element : layout.elements()) {
        if (element.is_wire()) {
            check_and_remove_segments(segment_map, element.element_id(),
                                      element.segment_tree());
        }
    }
    if (!segment_map.empty()) [[unlikely]] {
        throw_exception("selection contains segments that don't exist anymore");
    }
}

// Section

auto add_segment(Selection &selection, segment_t segment, const Layout &layout) -> void {
    const auto part = to_part(get_line(layout, segment));
    selection.add_segment(segment_part_t {segment, part});
}

auto add_segment_tree(Selection &selection, element_id_t element_id, const Layout &layout)
    -> void {
    if (!layout.element(element_id).is_wire()) {
        throw_exception("element needs to be wire");
    }

    const auto &tree = layout.segment_tree(element_id);
    for (const auto &segment_index : tree.indices()) {
        add_segment(selection, segment_t {element_id, segment_index}, layout);
    }
}

auto remove_segment(Selection &selection, segment_t segment, const Layout &layout)
    -> void {
    const auto part = to_part(get_line(layout, segment));
    selection.remove_segment(segment_part_t {segment, part});
}

auto remove_segment_tree(Selection &selection, element_id_t element_id,
                         const Layout &layout) -> void {
    if (!layout.element(element_id).is_wire()) {
        throw_exception("element needs to be wire");
    }

    const auto &tree = layout.segment_tree(element_id);
    for (const auto &segment_index : tree.indices()) {
        remove_segment(selection, segment_t {element_id, segment_index}, layout);
    }
}

auto add_segment_part(Selection &selection, const Layout &layout, segment_t segment,
                      point_fine_t point) -> void {
    const auto full_line = get_line(layout, segment);
    const auto parts = selection.selected_segments(segment);

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
    const auto parts = selection.selected_segments(segment);

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