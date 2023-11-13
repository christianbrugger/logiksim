#include "component/editable_circuit/visible_selection.h"

#include "allocated_size/std_vector.h"
#include "component/editable_circuit/layout_index.h"
#include "format/container.h"
#include "layout.h"
#include "layout_message.h"
#include "layout_message_generation.h"
#include "selection.h"
#include "selection_normalization.h"

#include <exception>

namespace logicsim {

template <>
auto format(SelectionFunction selection_function) -> std::string {
    switch (selection_function) {
        using enum SelectionFunction;

        //case toggle:
        //    return "toggle";
        case add:
            return "add";
        case substract:
            return "substract";
    }
    std::terminate();
}

//
// Selection Builder
//

auto SelectionBuilder::submit(const editable_circuit::InfoMessage& message) -> void {
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

auto SelectionBuilder::empty() const noexcept -> bool {
    return initial_selection_.empty() && operations_.empty();
}

auto SelectionBuilder::allocated_size() const -> std::size_t {
    return get_allocated_size(operations_);
}

auto SelectionBuilder::format() const -> std::string {
    return fmt::format("{}", operations_);
}

auto SelectionBuilder::clear() -> void {
    initial_selection_.clear();
    operations_.clear();
    cached_selection_.reset();
}

auto SelectionBuilder::add(SelectionFunction function, rect_fine_t rect) -> void {
    operations_.emplace_back(operation_t {function, rect});
    cached_selection_.reset();
}

auto SelectionBuilder::update_last(rect_fine_t rect) -> void {
    if (operations_.empty()) [[unlikely]] {
        throw std::runtime_error("Cannot update last with no operations.");
    }
    if (operations_.back().rect == rect) {
        return;
    }

    operations_.back().rect = rect;
    cached_selection_.reset();
}

auto SelectionBuilder::pop_last() -> void {
    if (operations_.empty()) [[unlikely]] {
        throw std::runtime_error("Cannot remove last with no operations.");
    }

    operations_.pop_back();
    cached_selection_.reset();
}

auto SelectionBuilder::set_selection(Selection selection) -> void {
    clear();
    initial_selection_.swap(selection);
}

namespace {

auto add_element_to_selection(logicitem_id_t logicitem_id, SelectionFunction function,
                              Selection& selection) {
    switch (function) {
        using enum SelectionFunction;

        case add: {
            selection.add(logicitem_id);
            return;
        }
        case substract: {
            selection.remove_logicitem(logicitem_id);
            return;
        }
        case toggle: {
            selection.toggle_logicitem(logicitem_id);
            return;
        }
    }
    std::terminate();
}

auto add_segment_to_selection(segment_t segment, SelectionBuilder::operation_t operation,
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
        case toggle: {
            throw std::runtime_error("not implemented");
        }
    }
    std::terminate();
}

auto apply_function(Selection& selection, const SelectionIndex& spatial_cache,
                    const Layout& layout, SelectionBuilder::operation_t operation)
    -> void {
    const auto selected_elements = spatial_cache.query_selection(operation.rect);

    for (auto&& element : selected_elements) {
        if (element.is_logicitem()) {
            add_element_to_selection(element.logicitem(), operation.function, selection);
        } else {
            add_segment_to_selection(element.segment(), operation, selection, layout);
        }
    }
}

}  // namespace

auto SelectionBuilder::calculate_selection() const -> Selection {
    auto selection = Selection {initial_selection_};

    for (auto&& operation : operations_) {
        apply_function(selection, layout_index_->spatial_cache(), *layout_, operation);

        if (operation.function == SelectionFunction::add) {
            sanitize_selection(selection, *layout_, layout_index_->collision_index(),
                               SanitizeMode::expand);
        }
        if (operation.function == SelectionFunction::substract) {
            sanitize_selection(selection, *layout_, layout_index_->collision_index(),
                               SanitizeMode::shrink);
        }
    }

    return selection;
}

auto SelectionBuilder::selection() const -> const Selection& {
    if (cached_selection_) {
        return *cached_selection_;
    }
    if (operations_.empty()) {
        return initial_selection_;
    }

    cached_selection_ = calculate_selection();
    return *cached_selection_;
}

auto SelectionBuilder::all_operations_applied() const -> bool {
    return operations_.empty();
}

auto SelectionBuilder::apply_all_operations() -> void {
    if (operations_.empty()) {
        return;
    }

    // update cache
    static_cast<void>(selection());

    if (cached_selection_) {
        initial_selection_.swap(*cached_selection_);
    }

    operations_.clear();
    cached_selection_.reset();
}

auto SelectionBuilder::clear_cache() const -> void {
    cached_selection_.reset();
}

auto SelectionBuilder::validate(const Layout& layout) const -> void {
    initial_selection_.validate(layout);

    if (cached_selection_) {
        cached_selection_->validate(layout);
    }

    calculate_selection().validate(layout);
}

}  // namespace logicsim
