#ifndef LOGICSIM_ALLOCATED_SIZE_STD_PAIR_H
#define LOGICSIM_ALLOCATED_SIZE_STD_PAIR_H

#include "allocated_size/trait.h"

#include <type_traits>
#include <utility>

namespace logicsim {

template <AllocatedSizeComputable T1, AllocatedSizeComputable T2>
    requires(!std::is_trivially_copyable_v<std::pair<T1, T2>>)
struct AllocatedSizeTrait<std::pair<T1, T2>> {
    using type = std::pair<T1, T2>;

    [[nodiscard]] static auto allocated_size(const type& pair) -> std::size_t {
        return get_allocated_size(pair.first) +  //
               get_allocated_size(pair.second);
    }
};

}  // namespace logicsim

#endif
