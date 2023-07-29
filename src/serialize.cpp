#include "serialize.h"

#include <glaze/glaze.hpp>

namespace logicsim {

auto serialize_json(const Layout &layout) -> void {
    print(layout.element_count());
}

}  // namespace logicsim
