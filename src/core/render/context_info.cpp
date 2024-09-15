#include "render/context_info.h"

#include "logging.h"
#include "vocabulary/context_render_settings.h"

#include <blend2d.h>

namespace logicsim {

auto context_info(const ContextRenderSettings& settings) -> BLContextCreateInfo {
    auto info = BLContextCreateInfo {};
    // lower the default value, so that Blend2D uses less memory when rendering
    // many small entities without compromising speed too much.
    info.commandQueueLimit = 2048;

    info.threadCount = gsl::narrow<decltype(info.threadCount)>(settings.thread_count);

    if (!settings.jit_rendering) {
        info.flags |= BL_CONTEXT_CREATE_FLAG_DISABLE_JIT;
    }
    return info;
}

auto equals(const BLContextCreateInfo& a, const BLContextCreateInfo& b) -> bool {
    static_assert(sizeof(BLContextCreateInfo) ==
                  sizeof(BLContextCreateInfo::flags) +
                      sizeof(BLContextCreateInfo::threadCount) +
                      sizeof(BLContextCreateInfo::cpuFeatures) +
                      sizeof(BLContextCreateInfo::commandQueueLimit) +
                      sizeof(BLContextCreateInfo::savedStateLimit) +
                      sizeof(BLContextCreateInfo::pixelOrigin) +
                      sizeof(BLContextCreateInfo::reserved));
    static_assert(std::extent_v<decltype(a.reserved)> == 1);

    return a.flags == b.flags &&                          //
           a.threadCount == b.threadCount &&              //
           a.cpuFeatures == b.cpuFeatures &&              //
           a.commandQueueLimit == b.commandQueueLimit &&  //
           a.savedStateLimit == b.savedStateLimit &&      //
           a.pixelOrigin == b.pixelOrigin &&              //
           a.reserved[0] == b.reserved[0];
}

}  // namespace logicsim
