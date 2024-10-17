#ifndef LOGICSIM_ALLOCATED_SIZE_ANKERL_UNORDERED_DENSE_H
#define LOGICSIM_ALLOCATED_SIZE_ANKERL_UNORDERED_DENSE_H

#include "core/allocated_size/std_pair.h"
#include "core/allocated_size/std_vector.h"
#include "core/allocated_size/trait.h"

#include <ankerl/unordered_dense.h>

namespace logicsim {

//
// map
//

// requires AllocatedSizeComputable<typename ankerl::unordered_dense::map<
// Key, T, Hash, KeyEqual, AllocatorOrContainer, Bucket>::value_container_type>

template <class Key, class T, class Hash, class KeyEqual, class AllocatorOrContainer,
          class Bucket>
struct AllocatedSizeTrait<
    ankerl::unordered_dense::map<Key, T, Hash, KeyEqual, AllocatorOrContainer, Bucket>> {
    using type = ankerl::unordered_dense::map<Key, T, Hash, KeyEqual,
                                              AllocatorOrContainer, Bucket>;

    [[nodiscard]] static auto allocated_size(const type& container) -> std::size_t {
        return get_allocated_size(container.values());
    }
};

//
// set
//

template <class Key, class Hash, class KeyEqual, class AllocatorOrContainer, class Bucket>
struct AllocatedSizeTrait<
    ankerl::unordered_dense::set<Key, Hash, KeyEqual, AllocatorOrContainer, Bucket>> {
    using type =
        ankerl::unordered_dense::set<Key, Hash, KeyEqual, AllocatorOrContainer, Bucket>;

    [[nodiscard]] static auto allocated_size(const type& container) -> std::size_t {
        return get_allocated_size(container.values());
    }
};

}  // namespace logicsim

#endif
