#include "component/editable_circuit/placeholder.h"

namespace logicsim {

namespace editable_circuit {

Placeholder::Placeholder() : Placeholder {Layout {}} {}

Placeholder::Placeholder(Layout&& layout__)
    : layout_ {std::move(layout__)},
      layout_index_ {layout_},
      selection_store_ {},
      visible_selection_ {} {}

}

}
