#ifndef LOGICSIM_TYPE_TRAIT_IS_EQUALITY_COMPARABLE_H
#define LOGICSIM_TYPE_TRAIT_IS_EQUALITY_COMPARABLE_H

#include <concepts>
#include <type_traits>

namespace logicsim {

//
// To std::true_type and std::false_type
//

template <typename T>
struct is_equality_comparable {
    using value = std::bool_constant<std::equality_comparable<T>>;
};

template <typename T>
using is_equality_comparable_v = is_equality_comparable<T>::value;

//
// To equality_comparable and not_equality_comparable tag types
//

struct equality_comparable;

struct not_equality_comparable;

template <typename T>
concept equality_comparable_tag = std::is_base_of_v<T, equality_comparable> ||
                                  std::is_base_of_v<T, equality_comparable>;

template <typename T>
struct to_equality_comparable_tag {
    using type = std::conditional_t<std::equality_comparable<T>, equality_comparable,
                                    not_equality_comparable>;
};

template <typename T>
using to_equality_comparable_tag_t = to_equality_comparable_tag<T>::type;

}  // namespace logicsim

#endif
