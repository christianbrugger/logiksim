#include "core/vocabulary/text_truncated.h"

namespace logicsim {

template <>
auto format(TextTruncated value) -> std::string {
    switch (value) {
        using enum TextTruncated;

        case yes: {
            return "yes";
        }
        case no: {
            return "no";
        }
    }
}

}  // namespace logicsim
