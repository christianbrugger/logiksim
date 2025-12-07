#include "core/render/context_info.h"

#include "core/vocabulary/context_render_settings.h"

#include <blend2d/blend2d.h>

namespace logicsim {

auto context_info(const ContextRenderSettings& settings) -> BLContextCreateInfo {
    auto info = BLContextCreateInfo {};
    // lower the default value, so that Blend2D uses less memory when rendering
    // many small entities without compromising speed too much.
    info.command_queue_limit = 2048;

    info.thread_count = [&]() -> uint32_t {
        switch (settings.thread_count) {
            using enum ThreadCount;
            case synchronous:
                return 0;
            case two:
                return 2;
            case four:
                return 4;
            case eight:
                return 8;
        };
        std::terminate();
    }();

    if (!settings.jit_rendering) {
        info.flags |= BL_CONTEXT_CREATE_FLAG_DISABLE_JIT;
    }
    return info;
}

auto equals(const BLContextCreateInfo& a, const BLContextCreateInfo& b) -> bool {
    static_assert(sizeof(BLContextCreateInfo) ==
                  sizeof(BLContextCreateInfo::flags) +
                      sizeof(BLContextCreateInfo::thread_count) +
                      sizeof(BLContextCreateInfo::cpu_features) +
                      sizeof(BLContextCreateInfo::command_queue_limit) +
                      sizeof(BLContextCreateInfo::saved_state_limit) +
                      sizeof(BLContextCreateInfo::pixel_origin) +
                      sizeof(BLContextCreateInfo::reserved));
    static_assert(std::extent_v<decltype(a.reserved)> == 1);

    return a.flags == b.flags &&                              //
           a.thread_count == b.thread_count &&                //
           a.cpu_features == b.cpu_features &&                //
           a.command_queue_limit == b.command_queue_limit &&  //
           a.saved_state_limit == b.saved_state_limit &&      //
           a.pixel_origin == b.pixel_origin &&                //
           a.reserved[0] == b.reserved[0];
}

}  // namespace logicsim
