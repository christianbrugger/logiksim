
#include "circuit_layout.h"

#include "exceptions.h"

namespace logicsim {

auto format(DisplayState state) -> std::string {
    switch (state) {
        using enum DisplayState;

        case normal:
            return "Normal";
        case selected:
            return "Selected";
        case new_unknown:
            return "NewUnknown";
        case new_valid:
            return "NewValid";
        case new_colliding:
            return "NewColliding";
    }
    throw_exception("Don't know how to convert ElementType to string.");
}

}  // namespace logicsim
