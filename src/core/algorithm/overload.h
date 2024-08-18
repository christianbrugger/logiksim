#ifndef LOGICSIM_ALGORITHM_OVERLOAD_H
#define LOGICSIM_ALGORITHM_OVERLOAD_H

namespace logicsim {

template <typename... Func>
struct overload : Func... {
    using Func::operator()...;
};

/**
 * brief: Overload callables for variant.
 */
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

}  // namespace logicsim

#endif
