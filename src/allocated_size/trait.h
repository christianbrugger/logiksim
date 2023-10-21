#ifndef LOGICSIM_ALLOCATED_SIZE_TRAIT_H
#define LOGICSIM_ALLOCATED_SIZE_TRAIT_H

#include <concepts>
#include <type_traits>

namespace logicsim {

template <typename T>
struct AllocatedSizeTrait {};

template <typename T>
concept HasAllocatedSizeTrait = requires(T obj) {
    { AllocatedSizeTrait<T>::allocated_size(obj) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept HasAllocatedSizeMember = requires(T obj) {
    { obj.allocated_size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept AllocatedSizeComputable = std::is_trivially_copyable_v<T> ||
                                  HasAllocatedSizeTrait<T> || HasAllocatedSizeMember<T>;

template <AllocatedSizeComputable T>
[[nodiscard]] auto get_allocated_size(const T& obj) -> std::size_t {
    if constexpr (HasAllocatedSizeTrait<T>) {
        return AllocatedSizeTrait<T>::allocated_size(obj);
    } else if constexpr (HasAllocatedSizeMember<T>) {
        return obj.allocated_size();
    } else {
        static_assert(std::is_trivially_copyable_v<T>);
        return sizeof(obj);
    }
}

}  // namespace logicsim

#endif
