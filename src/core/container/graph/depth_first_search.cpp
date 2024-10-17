#include "core/container/graph/depth_first_search.h"

#include <exception>

namespace logicsim {

template <>
auto format(DFSStatus result) -> std::string {
    switch (result) {
        using enum DFSStatus;

        case success:
            return "success";
        case unfinished_loop:
            return "unfinished_loop";
        case unfinished_disconnected:
            return "unfinished_disconnected";
    }
    std::terminate();
}

}  // namespace logicsim
