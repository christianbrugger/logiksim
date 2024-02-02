#include "component/editable_circuit/layout_modifier.h"

namespace logicsim {

namespace editable_circuit {

LayoutModifier::LayoutModifier(Layout&& layout__) : circuit_ {std::move(layout__)} {}

}  // namespace editable_circuit

}  // namespace logicsim
