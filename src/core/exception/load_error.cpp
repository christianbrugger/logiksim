#include "exception/load_error.h"

namespace logicsim {

LoadError::LoadError(std::string message) : message_ {std::move(message)} {}

auto LoadError::format() const -> const std::string& {
    return message_;
}

}  // namespace logicsim
