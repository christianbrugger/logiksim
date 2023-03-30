#ifndef LOGIKSIM_HASHING_H
#define LOGIKSIM_HASHING_H

#include <ankerl/unordered_dense.h>

namespace logicsim {

// standard secret from wyhash & ankerl
inline constexpr auto wyhash_secret = std::array {
    uint64_t {0xa0761d6478bd642f},
    uint64_t {0xe7037ed1a0b428db},
    uint64_t {0x8ebc6af09c88c6e3},
    uint64_t {0x589965cc75374cc3},
};

inline auto hash_16_byte(uint64_t a, uint64_t b) -> uint64_t {
    return ankerl::unordered_dense::detail::wyhash::mix(a ^ wyhash_secret[1],
                                                        b ^ wyhash_secret[0]);
}

inline auto hash_8_byte(uint32_t a, uint32_t b) -> uint64_t {
    auto packed = (uint64_t {a} << 32) + uint64_t {b};
    return ankerl::unordered_dense::detail::wyhash::hash(packed);
}

}  // namespace logicsim

#endif
