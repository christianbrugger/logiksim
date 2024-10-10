#include "vocabulary/size_2d.h"

namespace logicsim {

auto size_2d_t::format() const -> std::string {
    return fmt::format("[{}, {}]", width, height);
}

}  // namespace logicsim
