#ifndef LOGICSIM_RENDER_CONTEXT_INFO_H
#define LOGICSIM_RENDER_CONTEXT_INFO_H

struct BLContextCreateInfo;

namespace logicsim {

struct ContextRenderConfig;

auto context_info(const ContextRenderConfig& config) -> BLContextCreateInfo;

auto equals(const BLContextCreateInfo& a, const BLContextCreateInfo& b) -> bool;

}  // namespace logicsim

#endif
