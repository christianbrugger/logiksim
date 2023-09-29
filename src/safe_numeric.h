#ifndef LOGICSIM_SAFE_NUMERIC_H
#define LOGICSIM_SAFE_NUMERIC_H

#include <boost/safe_numerics/automatic.hpp>
#include <boost/safe_numerics/exception_policies.hpp>
#include <boost/safe_numerics/safe_base.hpp>
#include <boost/safe_numerics/safe_integer.hpp>

namespace logicsim {

using ls_promotion = boost::safe_numerics::automatic;

using ls_exception_policy = boost::safe_numerics::exception_policy<
    boost::safe_numerics::throw_exception,  // on_arithmetic_error
    boost::safe_numerics::throw_exception,  // on_implementation_defined_behavior
    boost::safe_numerics::throw_exception,  // on_undefined_behavior
    boost::safe_numerics::throw_exception   // on_uninitialized_value
    >;

template <class T>
using ls_safe = boost::safe_numerics::safe<T, ls_promotion, ls_exception_policy>;

template <class T, T Min, T Max>
using ls_safe_range =
    boost::safe_numerics::safe_base<T, Min, Max, ls_promotion, ls_exception_policy>;

}  // namespace logicsim

#endif
