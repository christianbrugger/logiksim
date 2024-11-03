#ifndef LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_VISIBLE_SELECTION_H
#define LOGIKSIM_COMPONENT_EDITABLE_CIRCUIT_VISIBLE_SELECTION_H

#include "core/format/struct.h"
#include "core/layout_message.h"
#include "core/selection.h"
#include "core/vocabulary/rect_fine.h"
#include "core/vocabulary/selection_function.h"

#include <optional>

namespace logicsim {

class Layout;
class SpatialIndex;
class LayoutIndex;

namespace visible_selection {

struct operation_t {
    SelectionFunction function {SelectionFunction::add};
    rect_fine_t rect {};

    [[nodiscard]] auto format() const -> std ::string;
    [[nodiscard]] auto operator==(const operation_t &) const -> bool = default;
};

static_assert(std::regular<operation_t>);

}  // namespace visible_selection

/**
 * @brief: Stores a visible selection, areas of positive and negative rectangles.
 *
 * Class-invariant:
 *   + cached_selection_ is only set if operations are non-empty
 */
class VisibleSelection {
   public:
    using operation_t = visible_selection::operation_t;

   public:
    [[nodiscard]] explicit VisibleSelection() = default;
    [[nodiscard]] explicit VisibleSelection(Selection selection);

    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto allocated_size() const -> std::size_t;
    [[nodiscard]] auto format() const -> std ::string;
    [[nodiscard]] auto operator==(const VisibleSelection &other) const -> bool;

    auto add(SelectionFunction function, rect_fine_t rect) -> void;
    auto update_last(rect_fine_t rect) -> void;
    auto pop_last() -> void;

    [[nodiscard]] auto selection(
        const Layout &layout, const LayoutIndex &layout_index) const -> const Selection &;
    [[nodiscard]] auto operations() const -> std::span<const operation_t>;
    [[nodiscard]] auto initial_selection() const -> const Selection &;

    auto submit(const InfoMessage &message) -> void;

   private:
    [[nodiscard]] auto class_invariant_holds() const -> bool;

   private:
    Selection initial_selection_ {};
    std::vector<operation_t> operations_ {};
    mutable std::optional<Selection> cached_selection_ {};
};

static_assert(std::regular<VisibleSelection>);

[[nodiscard]] auto last_operation(const VisibleSelection &visible_selection)
    -> std::optional<visible_selection::operation_t>;

}  // namespace logicsim

#endif
