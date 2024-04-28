#ifndef LOGICSIM_RENDER_MANAGED_CONTEXT_H
#define LOGICSIM_RENDER_MANAGED_CONTEXT_H

#include "render/bl_error_check.h"
#include "render/context2.h"

#include <blend2d.h>

#include <concepts>
#include <exception>

namespace logicsim {

// TODO move somewhere else ???
// TODO merge with bl_error_check & context_info ???
[[nodiscard]] auto create_context(BLImage &bl_image,
                                  const ContextRenderSettings &render_settings)
    -> BLContext;

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
    ContextData data_ {};
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
    auto context = Context {
        create_context(bl_image, data_.settings),
        std::move(data_),
    };
    const auto _ [[maybe_unused]] = gsl::finally([this, &context]() {
        static_assert(std::is_nothrow_move_assignable_v<decltype(data_)>);
        static_assert(!std::is_reference_v<decltype(context.extract_data())>);
        this->data_ = context.extract_data();
    });

    std::invoke(render_function, context);

    // In case of exception context.bl_ctx is cleaned up automatically and blocks
    // until all processing is done Here additional errors are checked.

    ensure_all_saves_restored(context.bl_ctx);
    check_errors(context.bl_ctx);
}

template <std::invocable<Context &> Func>
inline auto ImageContext::render(Func render_function) -> void {
    Expects(managed_context_.render_settings().view_config.size() == bl_image_.size());

    managed_context_.render(bl_image_, std::move(render_function));
}

}  // namespace logicsim

#endif
