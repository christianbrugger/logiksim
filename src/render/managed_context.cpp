#include "render/managed_context.h"

#include "render/bl_error_check.h"
#include "render/context_info.h"

#include <stdexcept>

namespace logicsim {

//
// Managed Context
//

auto ManagedContext::render_settings() const -> const ContextRenderSettings & {
    return context_.settings;
}

auto ManagedContext::set_render_settings(const ContextRenderSettings &new_settings)
    -> void {
    context_.settings = new_settings;
}

auto ManagedContext::clear() -> void {
    context_.clear();
}

auto ManagedContext::shrink_to_fit() -> void {
    context_.shrink_to_fit();
}

auto ManagedContext::begin(BLImage &bl_image) -> void {
    if (context_.settings.view_config.size() != bl_image.size()) {
        throw std::runtime_error("Given bl_image does not match size of settings.");
    }

    if (context_.bl_ctx.begin(bl_image, context_info(context_.settings)) != BL_SUCCESS) {
        throw std::runtime_error("Error during BLContext::begin");
    }
}

auto ManagedContext::end() -> void {
    checked_end(context_.bl_ctx);
}

//
// Image Context
//

auto ImageContext::render_settings() const -> const ContextRenderSettings & {
    return managed_context_.render_settings();
}

namespace {

auto resize_bl_image(BLImage &image, BLSizeI new_size) -> void {
    if (image.size() != new_size) {
        if (image.create(new_size.w, new_size.h, BL_FORMAT_PRGB32) != BL_SUCCESS) {
            throw std::runtime_error("Error while calling BLImage::create");
        }
    }
}

}  // namespace

auto ImageContext::set_render_settings(const ContextRenderSettings &new_settings)
    -> void {
    Expects(managed_context_.render_settings().view_config.size() == bl_image_.size());

    resize_bl_image(bl_image_, new_settings.view_config.size());
    managed_context_.set_render_settings(new_settings);

    Ensures(managed_context_.render_settings().view_config.size() == bl_image_.size());
}

auto ImageContext::bl_image() const -> const BLImage & {
    Expects(managed_context_.render_settings().view_config.size() == bl_image_.size());

    return bl_image_;
}

auto ImageContext::clear() -> void {}

auto ImageContext::shrink_to_fit() -> void {}

}  // namespace logicsim
