#ifndef LOGIKSIM_ALLOCATED_SIZE_H
#define LOGIKSIM_ALLOCATED_SIZE_H

#include "vocabulary.h"

namespace logicsim {

template <typename T>
struct AllocatingSizeTrait {};

template <typename T>
concept HasAllocatedSizeTrait = requires(T obj) {
                                    {
                                        AllocatingSizeTrait<T>::allocated_size(obj)
                                        } -> std::convertible_to<std::size_t>;
                                };
template <typename T>
concept HasAllocatedSizeMember = requires(T obj) {
                                     {
                                         obj.allocated_size()
                                         } -> std::convertible_to<std::size_t>;
                                 };

template <typename T>
concept HasAllocatedSize = HasAllocatedSizeTrait<T> || HasAllocatedSizeMember<T>;

template <typename T>
auto get_allocated_size(const T& obj) -> std::size_t {
    if constexpr (HasAllocatedSizeTrait<T>) {
        return AllocatingSizeTrait<T>::allocated_size(obj);
    } else if constexpr (HasAllocatedSizeMember<T>) {
        return obj.allocated_size();
    }
    return 0;
}

//
// std::vector
//

template <typename V>
struct AllocatingSizeTrait<std::vector<V>> {
    static auto allocated_size(const std::vector<V>& container) -> std::size_t {
        if constexpr (!HasAllocatedSize<V>) {
            return container.capacity() * sizeof(V);
        } else {
            auto count = std::size_t {container.capacity() * sizeof(V)};
            for (const auto& elem : container) {
                count += get_allocated_size(elem);
            }
            return count;
        }
    }
};

}  // namespace logicsim

#endif
