#include "core/vocabulary/orientation.h"

#include <exception>
#include <string>

namespace logicsim {

template <>
auto format(orientation_t orientation) -> std::string {
    switch (orientation) {
        using enum orientation_t;

        case right:
            return "right";
        case left:
            return "left";
        case up:
            return "up";
        case down:
            return "down";

        case undirected:
            return "undirected";
    }
    std::terminate();
}

auto is_directed(orientation_t orientation) -> bool {
    return orientation != orientation_t::undirected;
}

auto is_undirected(orientation_t orientation) -> bool {
    return orientation == orientation_t::undirected;
}

}  // namespace logicsim
