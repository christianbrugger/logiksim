#include "core/macro/try_catch.h"

namespace logicsim {

auto try_catch_non_empty_status() -> std::string {
#ifdef LS_DISABLE_ASAN_TRY_CATCH_NON_EMPTY
    return "Try-catch non-empty status: ASAN DISABLED";
#else
    return "Try-catch non-empty status: GOOD";
#endif
}

}  // namespace logicsim
