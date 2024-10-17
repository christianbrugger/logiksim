#ifndef LOGICSIM_CORE_VOCABULARY_THREAD_COUNT_H
#define LOGICSIM_CORE_VOCABULARY_THREAD_COUNT_H

#include "core/format/enum.h"

#include <cstdint>
#include <string>

namespace logicsim {

/**
 * @brief: Number of thread counts used for rendering.
 */
enum class ThreadCount : uint8_t {
    synchronous,
    two,
    four,
    eight,
};

template <>
[[nodiscard]] auto format(ThreadCount count) -> std::string;

}  // namespace logicsim

#endif
