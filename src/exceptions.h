#ifndef LOGIKSIM_EXCEPTIONS_H
#define LOGIKSIM_EXCEPTIONS_H

namespace logicsim {

/// consider using string literals
[[noreturn]] void throw_exception(const char* msg);

}  // namespace logicsim

#endif
