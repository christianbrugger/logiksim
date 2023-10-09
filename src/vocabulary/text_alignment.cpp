#include "vocabulary/text_alignment.h"

#include <exception>

namespace logicsim {

template <>
auto format(HorizontalAlignment alignment) -> std::string {
    switch (alignment) {
        using enum HorizontalAlignment;

        case left:
            return "left";
        case right:
            return "right";
        case center:
            return "center";
    }
    std::terminate();
}

template <>
auto format(VerticalAlignment alignment) -> std::string {
    switch (alignment) {
        using enum VerticalAlignment;

        case baseline:
            return "baseline";

        case center_baseline:
            return "center_baseline";
        case top_baseline:
            return "top_baseline";
        case bottom_baseline:
            return "bottom_baseline";

        case center:
            return "center";
        case top:
            return "top";
        case bottom:
            return "bottom";
    }
    std::terminate();
}

}  // namespace logicsim
