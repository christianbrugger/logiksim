#include "component/editable_circuit/visible_selection.h"

#include "allocated_size/std_vector.h"
#include "allocated_size/trait.h"
#include "component/editable_circuit/layout_index.h"
#include "format/container.h"
#include "format/std_type.h"
#include "layout.h"
#include "layout_message.h"
#include "selection.h"
#include "selection_normalization.h"

#include <exception>

namespace logicsim {

template <>
auto format(SelectionFunction selection_function) -> std::string {
    switch (selection_function) {
        using enum SelectionFunction;

        // case toggle:
        //     return "toggle";
        case add:
            return "add";
        case substract:
            return "substract";
    }
    std::terminate();
}

namespace visible_selection {

auto operation_t::format() const -> std::string {
    return fmt::format("operation_t(function = {}, rect = {})", function, rect);
}

}  // namespace visible_selection

//
// Selection Builder
//

auto VisibleSelection::submit(const editable_circuit::InfoMessage& message) -> void {
    using namespace editable_circuit::info_message;

    // we only keep the inital selection updated
    initial_selection_.submit(message);

    // we don't update our cache. In some cases we can't, as new elements are created.
    if (std::holds_alternative<LogicItemCreated>(message) ||
        std::holds_alternative<LogicItemIdUpdated>(message) ||
        std::holds_alternative<LogicItemDeleted>(message)) {
        clear_cache();
    }

    if (std::holds_alternative<SegmentCreated>(message) ||
        std::holds_alternative<SegmentIdUpdated>(message) ||
        std::holds_alternative<SegmentPartMoved>(message) ||
        std::holds_alternative<SegmentPartDeleted>(message)) {
        clear_cache();
    }
}

auto VisibleSelection::empty() const noexcept -> bool {
    return initial_selection_.empty() && operations_.empty();
}

auto VisibleSelection::allocated_size() const -> std::size_t {
    return get_allocated_size(initial_selection_) +  //
           get_allocated_size(operations_);
}

auto VisibleSelection::format() const -> std::string {
    return fmt::format(
        "VisibleSelection(\n"
        "  operations = {},\n"
        "  initial_selection = {}\n"
        ")",
        operations_, initial_selection_);
}

auto VisibleSelection::operator==(const VisibleSelection& other) const -> bool {
    // cache is not part of value type
    return initial_selection_ == other.initial_selection_ &&
           operations_ == other.operations_;
}

auto VisibleSelection::clear() -> void {
    initial_selection_.clear();
    operations_.clear();
    cached_selection_.reset();
}

auto VisibleSelection::add(SelectionFunction function, rect_fine_t rect) -> void {
    operations_.emplace_back(operation_t {function, rect});
    cached_selection_.reset();
}

auto VisibleSelection::update_last(rect_fine_t rect) -> void {
    if (operations_.empty()) [[unlikely]] {
        throw std::runtime_error("Cannot update last with no operations.");
    }
    if (operations_.back().rect == rect) {
        return;
    }

    operations_.back().rect = rect;
    cached_selection_.reset();
}

auto VisibleSelection::pop_last() -> void {
    if (operations_.empty()) [[unlikely]] {
        throw std::runtime_error("Cannot remove last with no operations.");
    }

    operations_.pop_back();
    cached_selection_.reset();
}

auto VisibleSelection::set_selection(Selection selection__) -> void {
    clear();
    initial_selection_ = std::move(selection__);
}

auto VisibleSelection::operation_count() const -> size_t {
    return operations_.size();
}

namespace {

auto add_element_to_selection(logicitem_id_t logicitem_id, SelectionFunction function,
                              Selection& selection) {
    switch (function) {
        using enum SelectionFunction;

        case add: {
            selection.add_logicitem(logicitem_id);
            return;
        }
        case substract: {
            selection.remove_logicitem(logicitem_id);
            return;
        }
    }
    std::terminate();
}

auto add_segment_to_selection(segment_t segment, VisibleSelection::operation_t operation,
                              Selection& selection, const Layout& layout) {
    const auto line = get_line(layout, segment);
    const auto part = to_part(line, operation.rect);

    if (!part) {
        return;
    }
    const auto segment_part = segment_part_t {segment, *part};

    switch (operation.function) {
        using enum SelectionFunction;

        case add: {
            selection.add_segment(segment_part);
            return;
        }
        case substract: {
            selection.remove_segment(segment_part);
            return;
        }
    }
    std::terminate();
}

auto apply_function(Selection& selection, const SpatialIndex& selection_index,
                    const Layout& layout, VisibleSelection::operation_t operation)
    -> void {
    const auto selected_elements = selection_index.query_selection(operation.rect);

    for (const auto& element : selected_elements) {
        if (element.is_logicitem()) {
            add_element_to_selection(element.logicitem(), operation.function, selection);
        } else {
            add_segment_to_selection(element.segment(), operation, selection, layout);
        }
    }
}

}  // namespace

auto VisibleSelection::calculate_selection(const Layout& layout,
                                           const LayoutIndex& layout_index) const
    -> Selection {
    auto selection = Selection {initial_selection_};

    for (auto&& operation : operations_) {
        apply_function(selection, layout_index.selection_index(), layout, operation);

        if (operation.function == SelectionFunction::add) {
            sanitize_selection(selection, layout, layout_index.collision_index(),
                               SanitizeMode::expand);
        }
        if (operation.function == SelectionFunction::substract) {
            sanitize_selection(selection, layout, layout_index.collision_index(),
                               SanitizeMode::shrink);
        }
    }

    return selection;
}

auto VisibleSelection::selection(const Layout& layout,
                                 const LayoutIndex& layout_index) const
    -> const Selection& {
    if (cached_selection_) {
        return *cached_selection_;
    }
    if (operations_.empty()) {
        return initial_selection_;
    }

    cached_selection_ = calculate_selection(layout, layout_index);
    return *cached_selection_;
}

auto VisibleSelection::apply_all_operations(const Layout& layout,
                                            const LayoutIndex& layout_index) -> void {
    if (operations_.empty()) {
        return;
    }

    // update cache
    static_cast<void>(selection(layout, layout_index));

    if (cached_selection_) {
        using std::swap;
        swap(initial_selection_, *cached_selection_);
    }

    operations_.clear();
    cached_selection_.reset();
}

auto VisibleSelection::clear_cache() const -> void {
    cached_selection_.reset();
}

}  // namespace logicsim