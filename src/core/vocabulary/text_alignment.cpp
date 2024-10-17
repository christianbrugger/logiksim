#include "core/vocabulary/text_alignment.h"

#include <exception>

namespace logicsim {

template <>
auto format(HTextAlignment alignment) -> std::string {
    switch (alignment) {
        using enum HTextAlignment;

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
auto format(VTextAlignment alignment) -> std::string {
    switch (alignment) {
        using enum VTextAlignment;

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
