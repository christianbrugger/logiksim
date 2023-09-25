#include "vocabulary/orientation.h"

#include <stdexcept>
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
    throw std::runtime_error("Don't know how to convert orientation_t to string.");
}

}  // namespace logicsim
