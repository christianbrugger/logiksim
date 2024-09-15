#ifndef LOGICSIM_RENDER_MANAGED_CONTEXT_H
#define LOGICSIM_RENDER_MANAGED_CONTEXT_H

#include "render/bl_error_check.h"
#include "render/write_file.h"
#include "render/context.h"
#include "render/context_guard.h"

#include <blend2d.h>

#include <concepts>
#include <exception>
#include <filesystem>

namespace logicsim {

// TODO !!! rename this file
// TODO !!! move free methods somewhere else ???
// TODO !!! merge with bl_error_check & context_info ???

/**
 * @brief: A Render Context that own the target image.
 */
class ImageSurface {
   public:
    [[nodiscard]] auto bl_image() const -> const BLImage &;

    /**
     * @brief: Renders the given function in the stored bl_image.
     *
     * Automatically resizes the bl_image as needed by the settings.
     */
    template <std::invocable<Context &> Func>
    inline auto render(const ContextRenderSettings &settings, ContextCache cache,
                       Func render_function) -> void;

   private:
    BLImage bl_image_ {};
};

/**
 * @brief: Create a context from the image and render settings.
 *
 * Throws an exception if the given image is not the size of the render settings.
 */
[[nodiscard]] auto create_context(
    BLImage &bl_image, const ContextRenderSettings &render_settings) -> BLContext;

/**
 * @brief: Allocates a new image if the size is different, without copying data.
 */
auto resize_image_no_copy(BLImage &image, BLSizeI new_size) -> void;

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

/**
 * @brief: Copies the data from the source layer to the target context.
 *
 * Throws if source and target don't have the same size.
 */
auto blit_layer(Context &target_ctx, const ImageSurface &source_layer,
                BLRectI dirty_rect) -> void;

/**
 * @brief: Renders the function first to the layer and then to the target within
 *         the given dirty_rect.
 */
template <std::invocable<Context &> Func>
auto render_layer(Context &target_ctx, ImageSurface &layer, BLRectI dirty_rect,
                  Func render_func) -> void;

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

template <std::invocable<Context &> Func>
inline auto ImageSurface::render(const ContextRenderSettings &settings,
                                 ContextCache cache, Func render_function) -> void {
    resize_image_no_copy(bl_image_, settings.view_config.size());
    render_to_image(bl_image_, settings, std::move(cache), std::move(render_function));
}

template <std::invocable<Context &> Func>
auto render_layer(Context &target_ctx, ImageSurface &layer, BLRectI dirty_rect,
                  Func render_func) -> void {
    layer.render(target_ctx.settings, target_ctx.cache, [&](Context &layer_ctx) {
        layer_ctx.bl_ctx.clearRect(dirty_rect);
        auto _ [[maybe_unused]] = make_context_guard(layer_ctx);
        std::invoke(render_func, layer_ctx);
    });

    blit_layer(target_ctx, layer, dirty_rect);
}

}  // namespace logicsim

#endif
