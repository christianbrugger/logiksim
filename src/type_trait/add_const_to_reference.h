#ifndef LOGICSIM_TYPE_TRAIT_ADD_CONST_TO_REFERENCE_H
#define LOGICSIM_TYPE_TRAIT_ADD_CONST_TO_REFERENCE_H

#include <type_traits>
#include <utility>

namespace logicsim {

template <typename T>
using add_const_to_reference_t = std::conditional_t<
    std::is_lvalue_reference_v<T>,
    std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>,
    std::add_const_t<T>>;

}

#endif
