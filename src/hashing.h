#ifndef LOGIKSIM_HASHING_H
#define LOGIKSIM_HASHING_H

#include <ankerl/unordered_dense.h>

namespace logicsim {

inline auto hash_16_byte(uint64_t a, uint64_t b) -> uint64_t {
    // standard secret from wyhash
    static constexpr auto secret = std::array {
        uint64_t {0xa0761d6478bd642f},
        uint64_t {0xe7037ed1a0b428db},
    };

    return ankerl::unordered_dense::detail::wyhash::mix(a ^ secret[1], b ^ secret[0]);
}

}  // namespace logicsim

#endif
