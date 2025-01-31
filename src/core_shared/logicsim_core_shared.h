#ifndef LOGICSIM_CORE_LOGICSIM_SHARED_H
#define LOGICSIM_CORE_LOGICSIM_SHARED_H

#ifdef _WIN32
#ifdef LS_CORE_LIB_BUILD_SHARED
#define LS_CORE_API __declspec(dllexport)
#else
#define LS_CORE_API __declspec(dllimport)
#endif
#else
#define LS_CORE_API
#endif

namespace logicsim {

[[nodiscard]] LS_CORE_API auto test() -> int;

}

#endif
