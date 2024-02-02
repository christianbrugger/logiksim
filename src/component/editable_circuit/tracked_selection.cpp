#include "component/editable_circuit/tracked_selection.h"

#include <fmt/core.h>
#include <gsl/gsl>

namespace logicsim {

namespace editable_circuit {

TrackedSelection::TrackedSelection(Selection&& selection__, const Layout& layout)
    : selection_ {std::move(selection__)} {
    Expects(is_selection_valid(selection_, layout));
}

auto TrackedSelection::format() const -> std::string {
    return fmt::format("Tracked-{}", selection_);
}

auto TrackedSelection::allocated_size() const -> std::size_t {
    return selection_.allocated_size();
}

auto TrackedSelection::selection() -> Selection& {
    return selection_;
}

auto TrackedSelection::selection() const -> const Selection& {
    return selection_;
}

}  // namespace editable_circuit

}  // namespace logicsim
