#ifndef LOGICSIM_ALLOCATED_SIZE_STD_STRING_H
#define LOGICSIM_ALLOCATED_SIZE_STD_STRING_H

#include "allocated_size/trait.h"

#include <string>
#include <type_traits>

namespace logicsim {

template <class CharT, class Traits, class Allocator>

struct AllocatedSizeTrait<std::basic_string<CharT, Traits, Allocator>> {
    using type = std::basic_string<CharT, Traits, Allocator>;

    [[nodiscard]] static auto is_inlined(const type& container) -> bool {
        // TODO check if this is defined behavior
        const auto data_ptr = static_cast<const void*>(container.data());
        const auto container_start = static_cast<const void*>(&container);
        const auto container_end = static_cast<const void*>(std::next(&container, 1));

        return (data_ptr >= container_start) && (data_ptr < container_end);
    }

    [[nodiscard]] static auto allocated_size(const type& container) -> std::size_t {
        if (is_inlined(container)) {
            return std::size_t {0};
        }
        // null-termination is not included in capacity
        return container.capacity() * sizeof(CharT) + sizeof(CharT);
    }
};

}  // namespace logicsim

#endif
