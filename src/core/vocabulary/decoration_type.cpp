#include "vocabulary/decoration_type.h"

#include <exception>
#include <string>

namespace logicsim {

template <>
auto format(DecorationType type) -> std::string {
    switch (type) {
        using enum DecorationType;

        case text_element:
            return "TextElement";
    }
    std::terminate();
}

}  // namespace logicsim
