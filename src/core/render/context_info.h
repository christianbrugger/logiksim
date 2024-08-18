#ifndef LOGICSIM_RENDER_CONTEXT_INFO_H
#define LOGICSIM_RENDER_CONTEXT_INFO_H

struct BLContextCreateInfo;

namespace logicsim {

struct ContextRenderSettings;

auto context_info(const ContextRenderSettings& settings) -> BLContextCreateInfo;

auto equals(const BLContextCreateInfo& a, const BLContextCreateInfo& b) -> bool;

}  // namespace logicsim

#endif
