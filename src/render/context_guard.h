#ifndef LOGICSIM_RENDER_CONTEXT_GUARD_H
#define LOGICSIM_RENDER_CONTEXT_GUARD_H

class BLContext;

namespace logicsim {

/**
 * @brief: An RAII type that saves and restores the context.
 *
 * This exists to make guarantee a restore even in case of exceptions.
 */
class ContextGuard {
   public:
    [[nodiscard]] explicit ContextGuard(BLContext& bl_ctx);
    ~ContextGuard();

   private:
    BLContext& bl_ctx_;
};

/**
 * @brief: Creates a new context guard
 *
 * This exists so guards can be created from other context types via ADL.
 */
[[nodiscard]] auto make_context_guard(BLContext& bl_ctx) -> ContextGuard;

}  // namespace logicsim

#endif
