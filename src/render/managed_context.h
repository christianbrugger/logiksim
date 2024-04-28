#ifndef LOGICSIM_RENDER_MANAGED_CONTEXT_H
#define LOGICSIM_RENDER_MANAGED_CONTEXT_H

#include "context2.h"

#include <blend2d.h>

#include <concepts>
#include <exception>

namespace logicsim {

/**
 * @brief: A managed context that renders to an outside source.
 */
class ManagedContext {
   public:
    [[nodiscard]] auto render_settings() const -> const ContextRenderSettings &;
    auto set_render_settings(const ContextRenderSettings &new_settings) -> void;

    /**
     * @brief: Sets up the Context and calls render_function.
     *
     * Throws an exception if the given image is not the size of the render settings.
     *
     * Note, render_function is not allowed to change Context::settings.
     */
    template <std::invocable<Context &> Func>
    inline auto render(BLImage &bl_image, Func render_function) -> void;

    auto clear() -> void;
    auto shrink_to_fit() -> void;

   private:
    auto begin(BLImage &bl_image) -> void;
    auto end() -> void;

    Context context_ {};
};

/**
 * @brief: A Render Context that own the target image.
 *
 * Class invariant:
 *   - bl_image always has the size as the render_settings().size()
 */
class ImageContext {
   public:
    [[nodiscard]] auto render_settings() const -> const ContextRenderSettings &;
    auto set_render_settings(const ContextRenderSettings &new_settings) -> void;

    [[nodiscard]] auto bl_image() const -> const BLImage &;

    /**
     * @brief: Renders the given function in the stored bl_image.
     *
     * Note, render_function is not allowed to change Context::settings.
     */
    template <std::invocable<Context &> Func>
    inline auto render(Func render_function) -> void;

    auto clear() -> void;
    auto shrink_to_fit() -> void;

   private:
    BLImage bl_image_ {};
    ManagedContext managed_context_ {};
};

//
// Implementation
//

template <std::invocable<Context &> Func>
inline auto ManagedContext::render(BLImage &bl_image, Func render_function) -> void {
    this->begin(bl_image);
    const auto settings = context_.settings;

    try {
        std::invoke(render_function, context_);
    } catch (...) {
        // TODO use std::throw_with_nested ???
        context_.bl_ctx.end();
        throw;
    }

    Expects(settings == context_.settings);
    this->end();
}

template <std::invocable<Context &> Func>
inline auto ImageContext::render(Func render_function) -> void {
    Expects(managed_context_.render_settings().view_config.size() == bl_image_.size());

    managed_context_.render(bl_image_, std::move(render_function));
}

}  // namespace logicsim

#endif
