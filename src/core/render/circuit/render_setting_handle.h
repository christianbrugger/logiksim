#ifndef LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_SETTING_HANDLE_H
#define LOGICSIM_CORE_RENDER_CIRCUIT_RENDER_SETTING_HANDLE_H

namespace logicsim {

class Layout;
class Selection;

struct Context;

auto render_setting_handle(Context& ctx, const Layout& layout, const Selection& selection)
    -> void;

}  // namespace logicsim

#endif
