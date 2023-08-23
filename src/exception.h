#ifndef LOGIKSIM_EXCEPTION_H
#define LOGIKSIM_EXCEPTION_H

#include <cstdlib>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

namespace detail {
[[noreturn]] auto throw_exception_impl(const char* msg) -> void;
}

[[noreturn]] constexpr auto throw_exception(const char* msg) -> void {
    detail::throw_exception_impl(msg);

    // if constexpr (std::is_constant_evaluated()) {
    //     std::abort();
    // } else {
    //     detail::throw_exception_impl(msg);
    // }
}

// constexpr void assert_ls(bool value, const char* msg = nullptr) {
// #ifndef NDEBUG
//     if (!value) {
//         if (msg == nullptr) {
//             throw_exception("assert exception!");
//         }
//         throw_exception(msg);
//     }
// #endif
// }

}  // namespace logicsim

#endif
