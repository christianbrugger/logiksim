#ifndef LOGIKSIM_RENDER_HELPER_H
#define LOGIKSIM_RENDER_HELPER_H

namespace logicsim {

class ContextGuard {
   public:
    [[nodiscard]] explicit ContextGuard(BLContext& bl_ctx);
    ~ContextGuard();

   private:
    BLContext& bl_ctx_;
};

[[nodiscard]] auto make_context_guard(BLContext& bl_ctx) -> ContextGuard;

}  // namespace logicsim

#endif