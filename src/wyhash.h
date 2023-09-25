#ifndef LOGIKSIM_HASHING_H
#define LOGIKSIM_HASHING_H

#include <ankerl/unordered_dense.h>

#include <bit>
#include <cstdint>

namespace logicsim {

// standard secret from wyhash & ankerl
constexpr inline auto wyhash_secret = std::array {
    uint64_t {0xa0761d6478bd642f},
    uint64_t {0xe7037ed1a0b428db},
    uint64_t {0x8ebc6af09c88c6e3},
    uint64_t {0x589965cc75374cc3},
};

[[nodiscard]] inline auto wyhash_128_bit(uint64_t a, uint64_t b) -> uint64_t {
    return ankerl::unordered_dense::detail::wyhash::mix(a ^ wyhash_secret[1],
                                                        b ^ wyhash_secret[0]);
}

[[nodiscard]] inline auto wyhash_64_bit(uint32_t a, uint32_t b) -> uint64_t {
    auto packed = (uint64_t {a} << 32) + uint64_t {b};
    return ankerl::unordered_dense::detail::wyhash::hash(packed);
}

[[nodiscard]] inline auto wyhash_64_bit(int32_t a, int32_t b) -> uint64_t {
    return wyhash_64_bit(std::bit_cast<uint32_t>(a), std::bit_cast<uint32_t>(b));
}

[[nodiscard]] inline auto wyhash_64_bit(uint32_t a, int32_t b) -> uint64_t {
    return wyhash_64_bit(std::bit_cast<uint32_t>(a), std::bit_cast<uint32_t>(b));
}

[[nodiscard]] inline auto wyhash_64_bit(int32_t a, uint32_t b) -> uint64_t {
    return wyhash_64_bit(std::bit_cast<uint32_t>(a), std::bit_cast<uint32_t>(b));
}

//
// 64 bit
//

[[nodiscard]] inline auto wyhash(uint64_t a) -> uint64_t {
    return ankerl::unordered_dense::detail::wyhash::hash(a);
}

[[nodiscard]] inline auto wyhash(int64_t a) -> uint64_t {
    return wyhash(std::bit_cast<uint64_t>(a));
}

//
// 32 bit
//

[[nodiscard]] inline auto wyhash(uint32_t a) -> uint64_t {
    return wyhash(static_cast<uint64_t>(a));
}

[[nodiscard]] inline auto wyhash(int32_t a) -> uint64_t {
    return wyhash(std::bit_cast<uint32_t>(a));
}

//
// 16 bit
//

[[nodiscard]] inline auto wyhash(uint16_t a) -> uint64_t {
    return wyhash(static_cast<uint64_t>(a));
}

[[nodiscard]] inline auto wyhash(int16_t a) -> uint64_t {
    return wyhash(std::bit_cast<uint16_t>(a));
}

//
// 8 bit
//

[[nodiscard]] inline auto wyhash(uint8_t a) -> uint64_t {
    return wyhash(static_cast<uint64_t>(a));
}

[[nodiscard]] inline auto wyhash(int8_t a) -> uint64_t {
    return wyhash(std::bit_cast<uint8_t>(a));
}

}  // namespace logicsim

#endif
