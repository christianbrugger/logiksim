#ifndef LOGICSIM_VOCABULARY_REALTIME_TIMEOUT_H
#define LOGICSIM_VOCABULARY_REALTIME_TIMEOUT_H

#include <chrono>

namespace logicsim {

using realtime_timeout_t = std::chrono::steady_clock::duration;

constexpr static auto no_realtime_timeout = realtime_timeout_t::max();

}  // namespace logicsim

#endif
