#ifndef LOGICSIM_ALLOCATED_SIZE_STD_OPTIONAL_H
#define LOGICSIM_ALLOCATED_SIZE_STD_OPTIONAL_H

#include "core/allocated_size/trait.h"

#include <optional>
#include <type_traits>

namespace logicsim {

template <AllocatedSizeComputable T>
    requires(!std::is_trivially_copyable_v<std::optional<T>>)
struct AllocatedSizeTrait<std::optional<T>> {
    using type = std::optional<T>;

    [[nodiscard]] static auto allocated_size(const type& pair) -> std::size_t {
        return pair.has_value() ? get_allocated_size(pair.value()) : std::size_t {0};
    }
};

}  // namespace logicsim

#endif
