#include "core/vocabulary/selection_function.h"

#include <fmt/format.h>

namespace logicsim {

template <>
auto format(SelectionFunction selection_function) -> std::string {
    switch (selection_function) {
        using enum SelectionFunction;

        case add:
            return "add";
        case substract:
            return "substract";
    }
    std::terminate();
}

}  // namespace logicsim
