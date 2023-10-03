#ifndef LOGICSIM_ALLOCATED_SIZE_STD_VECTOR_H
#define LOGICSIM_ALLOCATED_SIZE_STD_VECTOR_H

#include "allocated_size/trait.h"

#include <type_traits>
#include <vector>

namespace logicsim {

template <AllocatedSizeComputable Value>
struct AllocatedSizeTrait<std::vector<Value>> {
    using type = std::vector<Value>;

    [[nodiscard]] static auto allocated_size(const type& container) -> std::size_t {
        auto count = std::size_t {container.capacity() * sizeof(Value)};

        if constexpr (!std::is_trivially_copyable_v<Value>) {
            for (const auto& elem : container) {
                count += get_allocated_size(elem);
            }
        }

        return count;
    }
};

}  // namespace logicsim

#endif
