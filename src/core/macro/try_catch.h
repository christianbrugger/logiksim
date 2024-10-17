#ifndef LOGICSIM_CORE_MACRO_TRY_CATCH_H
#define LOGICSIM_CORE_MACRO_TRY_CATCH_H

#include <string>

namespace logicsim {

/**
 * @brief: Disable non empty exception catch for Windows Clang ASAN compilations.
 *
 * ASAN for Clang under Windows does not fully support exceptions.
 * As of version 18.1.8 exception handling via try catch is only supported,
 * if the catch block is basically empty.
 *
 * The non-empty try-catch blocks must be excluded from ASAN analysis via a
 * function / lambda annotation.
 *
 * References:
 *   https://github.com/google/sanitizers/issues/749
 *   https://stackoverflow.com/questions/76838834
 *
 * TODO: Check again with CLang 19. last check Clang 18.1.8.
 */
#if defined(__has_feature)
#if defined(__clang__) && defined(_WIN64) && __has_feature(address_sanitizer)
#define LS_DISABLE_ASAN_TRY_CATCH_NON_EMPTY
#endif
#endif

/**
 * @brief: Annotate all functions that contain a non-empty try-catch block.
 */
#ifdef LS_DISABLE_ASAN_TRY_CATCH_NON_EMPTY
#define LS_TRY_CATCH_NON_EMPTY __attribute__((no_sanitize_address))
#else
#define LS_TRY_CATCH_NON_EMPTY
#endif

/**
 * @brief: Get status of try_catch annotation.
 */
[[nodiscard]] auto try_catch_non_empty_status() -> std::string;

}  // namespace logicsim

#endif
