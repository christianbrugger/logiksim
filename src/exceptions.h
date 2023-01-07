#ifndef LOGIKSIM_EXCEPTIONS_H
#define LOGIKSIM_EXCEPTIONS_H

namespace logicsim {

/// consider using string literals

[[noreturn]] void throw_exception(const char* msg);

constexpr void assert_ls(bool value, const char* msg = nullptr) {
#ifndef NDEBUG
    if (!value) {
        if (msg == nullptr) {
            throw_exception("assert exception!");
        }
        throw_exception(msg);
    }
#endif
}

}  // namespace logicsim

#endif
