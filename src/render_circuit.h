#ifndef LOGIKSIM_RENDER_CIRCUIT_H
#define LOGIKSIM_RENDER_CIRCUIT_H

#include "renderer.h"  // TODO !!! remove, when done porting
#include "vocabulary.h"

namespace logicsim {

enum class shadow_t : uint8_t {
    selected,
    valid,
    colliding,
};

template <>
auto format(shadow_t state) -> std::string;

auto render_circuit_2(BLContext& ctx, render_args_t args) -> void;

}  // namespace logicsim

#endif