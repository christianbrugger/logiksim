#include "vocabulary/print_events.h"

#include <exception>

namespace logicsim {

template <>
auto format(PrintEvents type) -> std::string {
    switch (type) {
        using enum PrintEvents;

        case yes:
            return "yes";
        case no:
            return "no";
    }
    std::terminate();
}

}  // namespace logicsim
