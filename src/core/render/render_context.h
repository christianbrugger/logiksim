#ifndef LOGICSIM_CORE_RENDER_RENDER_CONTEXT_H
#define LOGICSIM_CORE_RENDER_RENDER_CONTEXT_H

#include "core/render/bl_error_check.h"
#include "core/render/context.h"
#include "core/render/context_cache.h"
#include "core/render/write_file.h"
#include "core/vocabulary/context_render_settings.h"

#include <blend2d.h>

#include <concepts>
#include <filesystem>

namespace logicsim {

/**
 * @brief: Create a context from the image and render settings.
 *
 * Throws an exception if the given image is not the size of the render settings.
 */
[[nodiscard]] auto create_context(
    BLImage &bl_image, const ContextRenderSettings &render_settings) -> BLContext;

/**
 * @brief: Create a context and calls render_function.
 *
 * Throws an exception if the given image is not the size of the render settings.
 */
template <std::invocable<Context &> Func>
inline auto render_to_image(BLImage &bl_image, const ContextRenderSettings &settings,
                            ContextCache cache, Func render_function) -> void;

/**
 * @brief: Renders the given function and stores the image to the file.
 *
 * Note only formats supported by BLImage::writeToFile() are supported.
 * At the moment *.png and *.qoi are supported.
 */
template <std::invocable<Context &> Func>
inline auto render_to_file(const std::filesystem::path &filename,
                           const ContextRenderSettings &settings, ContextCache cache,
                           Func render_function);

/**
 * @brief: Copies the data from the source image to the target context.
 *
 * Throws if source and target don't have the same size.
 */
auto blit_layer(Context &target_ctx, const BLImage &source_image,
                BLRectI dirty_rect) -> void;

//
// Implementation
//

template <std::invocable<Context &> Func>
inline auto render_to_image(BLImage &bl_image, const ContextRenderSettings &settings,
                            ContextCache cache, Func render_function) -> void {
    auto context = Context {
        .bl_ctx = create_context(bl_image, settings),
        .settings = settings,
        .cache = std::move(cache),
    };

    std::invoke(render_function, context);

    // In case of exception context.bl_ctx is cleaned up automatically and blocks
    // until all processing is done. Here additional errors are checked.

    ensure_all_saves_restored(context.bl_ctx);
    check_errors(context.bl_ctx);
}

template <std::invocable<Context &> Func>
inline auto render_to_file(const std::filesystem::path &filename,
                           const ContextRenderSettings &settings, ContextCache cache,
                           Func render_function) {
    const auto size = settings.view_config.size();
    auto bl_image = BLImage {size.w, size.h, BL_FORMAT_PRGB32};

    render_to_image(
        bl_image, settings, std::move(cache),
        [&render_function](Context &ctx) { std::invoke(render_function, ctx); });

    write_to_file(bl_image, filename);
}

}  // namespace logicsim

#endif
