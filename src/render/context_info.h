#ifndef LOGICSIM_RENDER_CONTEXT_INFO_H
#define LOGICSIM_RENDER_CONTEXT_INFO_H

struct BLContextCreateInfo;

namespace logicsim {

struct RenderSettings;

auto context_info(const RenderSettings& settings) -> BLContextCreateInfo;

auto equals(const BLContextCreateInfo& a, const BLContextCreateInfo& b) -> bool;

}  // namespace logicsim

#endif
