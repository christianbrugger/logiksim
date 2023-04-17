
#include "editable_circuit/selection_builder.h"

#include "algorithm.h"
#include "editable_circuit/caches.h"
#include "format.h"
#include "layout.h"
#include "range.h"
#include "timer.h"

namespace logicsim {

//
// Selection Builder
//

SelectionBuilder::SelectionBuilder(const Layout& layout,
                                   const CacheProvider& cache_provider)
    : layout_ {&layout}, cache_provider_ {&cache_provider} {}

auto SelectionBuilder::submit(editable_circuit::InfoMessage message) -> void {
    using namespace editable_circuit::info_message;

    // we only keep the inital selection updated
    initial_selection_.submit(message);

    // we don't update our cache. In some cases we can't, as new elements are created.
    if (std::holds_alternative<LogicItemCreated>(message)
        || std::holds_alternative<LogicItemIdUpdated>(message)
        || std::holds_alternative<LogicItemDeleted>(message)) {
        clear_cache();
    }

    if (std::holds_alternative<SegmentCreated>(message)
        || std::holds_alternative<SegmentIdUpdated>(message)
        || std::holds_alternative<SegmentPartMoved>(message)
        || std::holds_alternative<SegmentPartDeleted>(message)) {
        clear_cache();
    }
}

auto SelectionBuilder::empty() const noexcept -> bool {
    return initial_selection_.empty() && operations_.empty();
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
        throw_exception("Cannot update last with no operations.");
    }
    if (operations_.back().rect == rect) {
        return;
    }

    operations_.back().rect = rect;
    cached_selection_.reset();
}

auto SelectionBuilder::pop_last() -> void {
    if (operations_.empty()) [[unlikely]] {
        throw_exception("Cannot remove last with no operations.");
    }

    operations_.pop_back();
    cached_selection_.reset();
}

namespace {

auto add_element_to_selection(element_id_t element_id, SelectionFunction function,
                              Selection& selection) {
    switch (function) {
        using enum SelectionFunction;

        case add: {
            selection.add_logicitem(element_id);
            return;
        }
        case substract: {
            selection.remove_logicitem(element_id);
            return;
        }
        case toggle: {
            selection.toggle_logicitem(element_id);
            return;
        }
    }

    throw_exception("Unknown function");
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
            throw_exception("not implemented");
        }
    }

    throw_exception("Unknown function");
}

auto apply_function(Selection& selection, const SpatialTree& spatial_cache,
                    const Layout& layout, SelectionBuilder::operation_t operation)
    -> void {
    // const auto t = Timer {"apply_function", Timer::Unit::ms, 3};
    const auto selected_elements = spatial_cache.query_selection(operation.rect);

    for (auto&& element : selected_elements) {
        if (element.segment_index == null_segment_index) {
            add_element_to_selection(element.element_id, operation.function, selection);
        } else {
            const auto segment = segment_t {element.element_id, element.segment_index};
            add_segment_to_selection(segment, operation, selection, layout);
        }
    }
}

}  // namespace

auto SelectionBuilder::calculate_selection() const -> Selection {
    auto selection = Selection {initial_selection_};

    for (auto&& operation : operations_) {
        apply_function(selection, cache_provider_->spatial_cache(), *layout_, operation);
    }

    sanitize_selection(selection, *layout_, cache_provider_->collision_cache());
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

auto SelectionBuilder::create_selection_mask() const -> selection_mask_t {
    if (empty()) {
        return {};
    }

    // TODO create algorithm to mask?
    const auto element_count = layout_->element_count();
    auto mask = selection_mask_t(element_count, false);

    for (element_id_t element_id : selection().selected_logic_items()) {
        mask.at(element_id.value) = true;
    }

    return mask;
}

// auto SelectionBuilder::copy_selection() const -> selection_handle_t {
//     return editable_circuit_->create_selection(selection());
// }

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

auto SelectionBuilder::validate(const Circuit& circuit) const -> void {
    initial_selection_.validate(circuit);

    if (cached_selection_) {
        cached_selection_->validate(circuit);
    }

    calculate_selection().validate(circuit);
}

}  // namespace logicsim
