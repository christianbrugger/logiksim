#include "pch.h"

#include "main_winui/src/render_buffer.h"

#include "main_winui/src/ls_xaml_utils.h"

namespace logicsim {

SwapChainParams::SwapChainParams(const Params& params)
    : width_pixel_ {params.width_pixel},
      height_pixel_ {params.height_pixel},
      rasterization_scale_ {params.rasterization_scale} {
    if (width_pixel_ < 0 || height_pixel_ < 0 && rasterization_scale_ <= 0) {
        throw std::runtime_error("invalid parameters");
    }

    Ensures(width_pixel_ >= 0);
    Ensures(height_pixel_ >= 0);
    Ensures(rasterization_scale_ > 0);
}

auto SwapChainParams::width_pixel() const -> int32_t {
    Expects(width_pixel_ >= 0);
    return width_pixel_;
}

auto SwapChainParams::height_pixel() const -> int32_t {
    Expects(height_pixel_ >= 0);
    return height_pixel_;
}

auto SwapChainParams::rasterization_scale() const -> double {
    Expects(rasterization_scale_ > 0);
    return rasterization_scale_;
}

auto SwapChainParams::dpi() const -> float {
    Expects(rasterization_scale_ > 0);
    const auto result = static_cast<float>(rasterization_scale_ * LS_IDENTITY_DPI);
    Ensures(result > 0);
    return result;
}

auto SwapChainParams::width_device() const -> float {
    Expects(width_pixel_ >= 0);
    Expects(rasterization_scale_ > 0);
    const auto result = static_cast<float>(width_pixel_ / rasterization_scale_);
    Ensures(result >= 0);
    return result;
}

auto SwapChainParams::height_device() const -> float {
    Expects(height_pixel_ >= 0);
    Expects(rasterization_scale_ > 0);
    const auto result = static_cast<float>(height_pixel_ / rasterization_scale_);
    Ensures(result >= 0);
    return result;
}

auto to_swap_chain_params(const CanvasParams& params) -> std::optional<SwapChainParams> {
    if (params.width_device < 0 ||
        params.height_device < 0 && params.rasterization_scale <= 0) {
        return std::nullopt;
    }

    const auto result = SwapChainParams {SwapChainParams::Params {
        .width_pixel = gsl::narrow<int32_t>(
            std::round(params.width_device * params.rasterization_scale)),
        .height_pixel = gsl::narrow<int32_t>(
            std::round(params.height_device * params.rasterization_scale)),
        .rasterization_scale = params.rasterization_scale,
    }};

    const auto dx_pixel = std::abs(params.height_device - result.height_device()) *
                          params.rasterization_scale;
    const auto dy_pixel = std::abs(params.height_device - result.height_device()) *
                          params.rasterization_scale;

    if (dx_pixel >= 0.01 || dy_pixel >= 0.01) {
        OutputDebugStringW(L"WARNING: canvas size is not aligned to pixels:");
        OutputDebugStringW(std::format(L"WARNING: width_device = {}, height_device = {}, "
                                       L"rasterization_scale = {}",
                                       params.width_device, params.height_device,
                                       params.rasterization_scale)
                               .c_str());
    }

    return result;
}

auto to_swap_chain_params_or_default(const CanvasParams& params) -> SwapChainParams {
    if (const auto result = to_swap_chain_params(params)) {
        return result.value();
    }
    return SwapChainParams {};
}

auto frame_buffer_size(const SwapChainParams& params) -> std::size_t {
    const auto w = static_cast<std::size_t>(params.width_pixel());
    const auto h = static_cast<std::size_t>(params.height_pixel());
    const auto m = static_cast<std::size_t>(LS_CANVAS_COLOR_BYTES);

    // ignore potential overflow for now
    return w * h * m;
}

auto to_point_pixel(const PointDevice& point,
                    const SwapChainParams& params) -> PointPixel {
    const auto scale = params.rasterization_scale();

    return PointPixel {
        .x = point.x * scale,
        .y = point.y * scale,
    };
}

auto to_point_pixel_int(const PointDevice& point,
                        const SwapChainParams& params) -> PointPixelInt {
    return to_point_pixel_int(to_point_pixel(point, params));
}

namespace concurrent_buffer {

//
// ConcurrentSwapChainParams
//

[[nodiscard]] auto ConcurrentSwapChainParams::get() const -> SwapChainParams {
    const auto _ [[maybe_unused]] = std::shared_lock(mutex_);
    return value_;
}

auto ConcurrentSwapChainParams::set(const SwapChainParams& new_value) -> void {
    const auto _ [[maybe_unused]] = std::lock_guard(mutex_);
    value_ = new_value;
}

//
// BufferQueue
//

[[nodiscard]] auto BufferQueue::empty() const -> bool {
    return buffer_.empty();
}

[[nodiscard]] auto BufferQueue::size() const -> std::size_t {
    return buffer_.size();
}

auto BufferQueue::push(value_type&& value) -> void {
    buffer_.push_back(std::move(value));
}

auto BufferQueue::push_front(value_type&& value) -> void {
    buffer_.insert(buffer_.begin(), std::move(value));
}

auto BufferQueue::pop() -> value_type {
    Expects(!buffer_.empty());

    auto result = value_type {std::move(buffer_.front())};
    buffer_.erase(buffer_.begin());

    return result;
}

//
// ConcurrentBufferQueue
//

auto ConcurrentBufferQueue::push(value_type&& value) -> void {
    {
        const auto _ [[maybe_unused]] = std::lock_guard(queue_mutex_);
        queue_.push(std::move(value));
    }
    queue_cv_.notify_one();
}

auto ConcurrentBufferQueue::push_front(value_type&& value) -> void {
    {
        const auto _ [[maybe_unused]] = std::lock_guard(queue_mutex_);
        queue_.push_front(std::move(value));
    }
    queue_cv_.notify_one();
}

auto ConcurrentBufferQueue::pop() -> value_type {
    auto lock = std::unique_lock(queue_mutex_);
    queue_cv_.wait(lock, [&]() { return shutdown_ || !queue_.empty(); });

    if (shutdown_) {
        throw ShutdownException {"ConcurrentMoveBlockingQueue shutdown."};
    }

    return queue_.pop();
}

auto ConcurrentBufferQueue::shutdown() -> void {
    {
        const auto _ [[maybe_unused]] = std::lock_guard(queue_mutex_);
        shutdown_ = true;
    }
    queue_cv_.notify_all();
}

auto ConcurrentBufferQueue::unsafe_size() const -> std::size_t {
    return queue_.size();
}

}  // namespace concurrent_buffer

//
// ConcurrentBuffer
//

ConcurrentBuffer::ConcurrentBuffer(int count) {
    Expects(count >= 1);

    for (const auto _ [[maybe_unused]] : std::ranges::views::iota(0, count)) {
        ready_to_fill_.push(std::make_unique<concurrent_buffer::RenderBuffer>());
    }

    Ensures(ready_to_fill_.unsafe_size() == gsl::narrow<std::size_t>(count));
    Ensures(ready_to_present_.unsafe_size() == 0);
}

auto ConcurrentBuffer::params() const -> SwapChainParams {
    return new_params_.get();
}

auto ConcurrentBuffer::update_params(const SwapChainParams& new_params) -> void {
    new_params_.set(new_params);
}

auto ConcurrentBuffer::shutdown() -> void {
    ready_to_fill_.shutdown();
    ready_to_present_.shutdown();
}

//
// Shared Concurrent Buffer
//

RenderBufferSource::RenderBufferSource(std::shared_ptr<ConcurrentBuffer> buffer)
    : buffer_ {std::move(buffer)} {
    Ensures(buffer_);
}

auto RenderBufferSource::params() const -> SwapChainParams {
    Expects(buffer_);
    return buffer_->params();
}

auto RenderBufferSource::update_params(const SwapChainParams& new_params) -> void {
    Expects(buffer_);
    buffer_->update_params(new_params);
}

RenderBufferSink::RenderBufferSink(std::shared_ptr<ConcurrentBuffer> buffer)
    : buffer_ {std::move(buffer)} {
    Ensures(buffer_);
}

RenderBufferControl::RenderBufferControl(std::shared_ptr<ConcurrentBuffer> buffer)
    : buffer_ {std::move(buffer)} {
    Ensures(buffer_);
}

RenderBufferControl::~RenderBufferControl() {
    if (buffer_) {
        buffer_->shutdown();
    }
}

auto create_render_buffer_parts(int count) -> RenderBufferParts {
    auto buffer = std::make_shared<ConcurrentBuffer>(count);

    return RenderBufferParts {
        .source = RenderBufferSource {buffer},
        .sink = RenderBufferSink {buffer},
        .control = RenderBufferControl {buffer},
    };
}

}  // namespace logicsim