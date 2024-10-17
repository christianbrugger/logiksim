#include "core/vocabulary/thread_count.h"

namespace logicsim {

template <>
auto format(ThreadCount count) -> std::string {
    switch (count) {
        using enum ThreadCount;
        case synchronous:
            return "synchronous";
        case two:
            return "two";
        case four:
            return "four";
        case eight:
            return "eight";
    };
    std::terminate();
}

}  // namespace logicsim
