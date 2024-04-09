#include "vocabulary/direction_type.h"

#include <exception>

namespace logicsim {

template <>
auto format(DirectionType type) -> std::string {
    switch (type) {
        using enum DirectionType;

        case undirected:
            return "undirected";
        case directed:
            return "directed";
        case any:
            return "any";
    }
    std::terminate();
}

}  // namespace logicsim
