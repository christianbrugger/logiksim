#ifndef LOGICSIM_ALLOCATED_SIZE_FOLLY_SMALL_VECTOR_H
#define LOGICSIM_ALLOCATED_SIZE_FOLLY_SMALL_VECTOR_H

#include "allocated_size/trait.h"

#include <folly/small_vector.h>

#include <type_traits>

namespace logicsim {

template <AllocatedSizeComputable Value, std::size_t RequestedMaxInline, class Policy>
struct AllocatedSizeTrait<folly::small_vector<Value, RequestedMaxInline, Policy>> {
    using type = folly::small_vector<Value, RequestedMaxInline, Policy>;

    [[nodiscard]] static auto is_inlined(const type& container) -> bool {
        // TODO check if this is defined behavior
        const auto data_ptr = static_cast<const void*>(container.data());
        const auto container_start = static_cast<const void*>(&container);
        const auto container_end = static_cast<const void*>(std::next(&container, 1));

        return (data_ptr >= container_start) && (data_ptr < container_end);
    }

    [[nodiscard]] static auto allocated_size(const type& container) -> std::size_t {
        auto count = is_inlined(container) ? 0 : container.capacity() * sizeof(Value);

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
