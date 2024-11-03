#include "core/component/editable_circuit/visible_selection.h"

#include "core/allocated_size/std_optional.h"
#include "core/allocated_size/std_vector.h"
#include "core/allocated_size/trait.h"
#include "core/component/editable_circuit/layout_index.h"
#include "core/component/editable_circuit/visible_selection.h"
#include "core/format/container.h"
#include "core/format/std_type.h"
#include "core/layout.h"
#include "core/layout_message.h"
#include "core/selection.h"
#include "core/selection_normalization.h"

#include <gsl/gsl>

#include <exception>

namespace logicsim {

namespace visible_selection {

auto operation_t::format() const -> std::string {
    return fmt::format("operation_t(function = {}, rect = {})", function, rect);
}

}  // namespace visible_selection

//
// Visible Selection
//

VisibleSelection::VisibleSelection(Selection selection)
    : initial_selection_ {std::move(selection)} {
    Ensures(class_invariant_holds());
}

auto VisibleSelection::submit(const InfoMessage& message) -> void {
    using namespace info_message;
    Expects(class_invariant_holds());

    // we only keep the inital selection updated
    initial_selection_.submit(message);

    // we don't update our cache. In some cases we can't (as new elements are created).
    cached_selection_.reset();

    Ensures(class_invariant_holds());
}

auto VisibleSelection::empty() const noexcept -> bool {
    Expects(class_invariant_holds());

    return initial_selection_.empty() && operations_.empty();
}

auto VisibleSelection::allocated_size() const -> std::size_t {
    Expects(class_invariant_holds());

    return get_allocated_size(initial_selection_) +  //
           get_allocated_size(operations_) +         //
           get_allocated_size(cached_selection_);
}

auto VisibleSelection::format() const -> std::string {
    Expects(class_invariant_holds());

    return fmt::format(
        "VisibleSelection(\n"
        "  operations = {},\n"
        "  initial_selection = {}\n"
        ")",
        operations_, initial_selection_);
}

auto VisibleSelection::operator==(const VisibleSelection& other) const -> bool {
    Expects(class_invariant_holds());

    // cache is not part of value type
    return initial_selection_ == other.initial_selection_ &&
           operations_ == other.operations_;
}

auto VisibleSelection::add(SelectionFunction function, rect_fine_t rect) -> void {
    Expects(class_invariant_holds());

    operations_.emplace_back(operation_t {function, rect});
    cached_selection_.reset();

    Ensures(class_invariant_holds());
}

auto VisibleSelection::update_last(rect_fine_t rect) -> void {
    Expects(class_invariant_holds());

    if (operations_.empty()) [[unlikely]] {
        throw std::runtime_error("Cannot update last with no operations.");
    }
    if (operations_.back().rect == rect) {
        return;
    }

    operations_.back().rect = rect;
    cached_selection_.reset();

    Ensures(class_invariant_holds());
}

auto VisibleSelection::pop_last() -> void {
    Expects(class_invariant_holds());

    if (operations_.empty()) [[unlikely]] {
        throw std::runtime_error("Cannot remove last with no operations.");
    }

    operations_.pop_back();
    cached_selection_.reset();

    Ensures(class_invariant_holds());
}

auto VisibleSelection::operations() const -> std::span<const operation_t> {
    Expects(class_invariant_holds());

    return operations_;
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

auto add_element_to_selection(decoration_id_t decoration_id, SelectionFunction function,
                              Selection& selection) {
    switch (function) {
        using enum SelectionFunction;

        case add: {
            selection.add_decoration(decoration_id);
            return;
        }
        case substract: {
            selection.remove_decoration(decoration_id);
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
                    const Layout& layout,
                    VisibleSelection::operation_t operation) -> void {
    const auto selected_elements = selection_index.query_selection(operation.rect);

    for (const auto& element : selected_elements) {
        if (element.is_logicitem()) {
            add_element_to_selection(element.logicitem(), operation.function, selection);
        } else if (element.is_segment()) {
            add_segment_to_selection(element.segment(), operation, selection, layout);
        } else if (element.is_decoration()) {
            add_element_to_selection(element.decoration(), operation.function, selection);
        }
    }
}

using operation_span_t = std::span<const visible_selection::operation_t>;

[[nodiscard]] auto calculate_selection(const Selection& initial_selection,
                                       operation_span_t operations, const Layout& layout,
                                       const LayoutIndex& layout_index) -> Selection {
    auto selection = Selection {initial_selection};

    for (const auto& operation : operations) {
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

}  // namespace

auto VisibleSelection::selection(
    const Layout& layout, const LayoutIndex& layout_index) const -> const Selection& {
    Expects(class_invariant_holds());

    if (cached_selection_) {
        // expects cache is up to date in debug
        assert(cached_selection_ == calculate_selection(initial_selection_, operations_,
                                                        layout, layout_index));
        return *cached_selection_;
    }
    if (operations_.empty()) {
        return initial_selection_;
    }

    cached_selection_ =
        calculate_selection(initial_selection_, operations_, layout, layout_index);

    Ensures(class_invariant_holds());
    return cached_selection_.value();
}

auto VisibleSelection::initial_selection() const -> const Selection& {
    Expects(class_invariant_holds());

    return initial_selection_;
}

auto VisibleSelection::class_invariant_holds() const -> bool {
    Expects(!cached_selection_.has_value() || !operations_.empty());

    return true;
}

//
// Free Functions
//

auto last_operation(const VisibleSelection& visible_selection)
    -> std::optional<visible_selection::operation_t> {
    const auto& operations = visible_selection.operations();

    if (operations.empty()) {
        return std::nullopt;
    }
    return operations.back();
}

auto operation_count(const VisibleSelection& visible_selection) -> std::size_t {
    return visible_selection.operations().size();
}

}  // namespace logicsim
