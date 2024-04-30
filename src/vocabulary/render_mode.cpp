#include "vocabulary/render_mode.h"

#include <exception>

namespace logicsim {

template <>
auto format(RenderMode type) -> std::string {
    switch (type) {
        using enum RenderMode;

        case direct:
            return "direct";
        case buffered:
            return "buffered";
    }
    std::terminate();
}

}  // namespace logicsim
