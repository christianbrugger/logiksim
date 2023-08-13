#ifndef LOGIKSIM_EXCEPTION_H
#define LOGIKSIM_EXCEPTION_H

namespace logicsim {

/// consider using string literals

[[noreturn]] auto throw_exception(const char* msg) -> void;

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
