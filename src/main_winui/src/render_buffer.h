#pragma once

#include "main_winui/src/ls_vocabulary.h"

#include <gsl/gsl>

#include <winrt/Microsoft.Graphics.Canvas.h>

#include <ranges>
#include <shared_mutex>
#include <vector>

namespace logicsim {

// CanvasSwapChain Default: B8G8R8A8UIntNormalized
constexpr static inline auto LS_CANVAS_PIXEL_FORMAT =
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized;
// For B8G8R8A8UIntNormalized: 4
constexpr static inline auto LS_CANVAS_COLOR_BYTES = int32_t {4};
// CanvasSwapChain Default: Premultiplied
constexpr static inline auto LS_CANVAS_ALPHA_MODE =
    winrt::Microsoft::Graphics::Canvas::CanvasAlphaMode::Premultiplied;
// CanvasSwapChain Default: 2  (allowed 2 - 16)
constexpr static inline auto LS_CANVAS_BUFFER_COUNT = int32_t {2};
// Fastest is no-vsync: 0 (0 - 4)
constexpr static inline auto LS_CANVAS_SYNC_INTERVAL = int32_t {0};
// For best performance: 2  (allowed 1+)
constexpr static inline auto LS_RENDER_BUFFER_DEFAULT_QUEUE_SIZE = int {2};

//
// SwapChain Parameters
//

using frame_t = std::vector<uint8_t>;

struct CanvasParams {
    float width_device {0};
    float height_device {0};
    double rasterization_scale {1.0};

    [[nodiscard]] auto operator==(const CanvasParams&) const -> bool = default;
};

/**
 * @brief:
 *
 * Class-invariants:
 *   + width_pixel_ >= 0
 *   + height_pixel_ >= 0
 *   + rasterization_scale_ > 0
 */
class SwapChainParams {
   public:
    struct Params {
        int32_t width_pixel {0};
        int32_t height_pixel {0};
        double rasterization_scale {1.0};

        [[nodiscard]] auto operator==(const Params&) const -> bool = default;
    };

    explicit SwapChainParams() = default;
    explicit SwapChainParams(const Params& params);

    [[nodiscard]] auto operator==(const SwapChainParams&) const -> bool = default;

    [[nodiscard]] auto width_pixel() const -> int32_t;
    [[nodiscard]] auto height_pixel() const -> int32_t;
    [[nodiscard]] auto rasterization_scale() const -> double;

    [[nodiscard]] auto dpi() const -> float;
    [[nodiscard]] auto width_device() const -> float;
    [[nodiscard]] auto height_device() const -> float;

   private:
    int32_t width_pixel_ {0};
    int32_t height_pixel_ {0};
    double rasterization_scale_ {1.0};
};

[[nodiscard]] auto to_swap_chain_params(const CanvasParams& params)
    -> std::optional<SwapChainParams>;

[[nodiscard]] auto to_swap_chain_params_or_default(const CanvasParams& params)
    -> SwapChainParams;

[[nodiscard]] auto frame_buffer_size(const SwapChainParams& params) -> std::size_t;

//
// Point Definition
//

[[nodiscard]] auto to_point_pixel(const PointDevice& point,
                                  const SwapChainParams& params) -> PointPixel;
[[nodiscard]] auto to_point_pixel_int(const PointDevice& point,
                                      const SwapChainParams& params) -> PointPixelInt;

namespace concurrent_buffer {

class ConcurrentSwapChainParams {
   public:
    [[nodiscard]] auto get() const -> SwapChainParams;
    auto set(const SwapChainParams& new_value) -> void;

   private:
    mutable std::shared_mutex mutex_ {};
    SwapChainParams value_ {};
};

class RenderBuffer {
   public:
    RenderBuffer() = default;
    ~RenderBuffer() = default;

    // delete anyway because of mutex
    RenderBuffer(RenderBuffer&&) = delete;
    auto operator=(RenderBuffer&&) -> RenderBuffer& = delete;

    // make non-copyable, so we know that queue operates efficiently
    RenderBuffer(const RenderBuffer&) = delete;
    auto operator=(const RenderBuffer&) -> RenderBuffer& = delete;

    template <std::invocable<SwapChainParams&, frame_t&> Func>
    auto modify(Func func);

   private:
    std::mutex mutex_ {};
    SwapChainParams params_ {};
    frame_t data_ {};
};

/**
 * @brief: Queue optimized for small sizes.
 *
 * Note the std::queue is not used, as it uses the expensive std::deque implementation.
 * It only makes sense for large queue sizes, while the Render buffer uses << 10 items.
 *
 * Specifically non-generic as the use-case is niche and implementation small.
 */
class BufferQueue {
   public:
    using value_type = std::unique_ptr<RenderBuffer>;

   public:
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    auto push(value_type&& value) -> void;
    auto push_front(value_type&& value) -> void;
    auto pop() -> value_type;

   private:
    std ::vector<value_type> buffer_;
};

/**
 * @brief: Thread-safe queue with blocking pop.
 *
 * Class-invariants:
 *   + shutdown is never set to false, once set to true.
 */
class ConcurrentBufferQueue {
   public:
    using value_type = std::unique_ptr<RenderBuffer>;

   public:
    auto push(value_type&& value) -> void;
    auto push_front(value_type&& value) -> void;

    /**
     * @brief: Returns next queue item.
     *
     * Blocks until an entry is available or shutdown is initiaed.
     *
     * Throws ShutdownException, if queue is shut down.
     */
    auto pop() -> value_type;

    auto shutdown() -> void;

    [[nodiscard]] auto unsafe_size() const -> std::size_t;

   private:
    mutable std::mutex queue_mutex_ {};
    std::condition_variable queue_cv_ {};

    BufferQueue queue_ {};
    bool shutdown_ {false};
};

}  // namespace concurrent_buffer

enum class BufferDrawStatus {
    DrawingSucceded,
    DrawingFailed,
};

/**
 * @Brief: Thread-safe Render Buffer.
 */
class ConcurrentBuffer {
   public:
    explicit ConcurrentBuffer(int count = LS_RENDER_BUFFER_DEFAULT_QUEUE_SIZE);

    template <std::invocable<const SwapChainParams&, frame_t&> Func>
    auto render_to_buffer(Func fun) -> void;

    template <std::invocable<const SwapChainParams&, const frame_t&> Func>
        requires std::same_as<
                     std::invoke_result_t<Func, const SwapChainParams&, frame_t&>,
                     BufferDrawStatus>
    auto draw_buffer(Func func) -> BufferDrawStatus;

    [[nodiscard]] auto params() const -> SwapChainParams;
    auto update_params(const SwapChainParams& new_params) -> void;
    auto shutdown() -> void;

   private:
    concurrent_buffer::ConcurrentSwapChainParams new_params_ {};

    concurrent_buffer::ConcurrentBufferQueue ready_to_fill_ {};
    concurrent_buffer::ConcurrentBufferQueue ready_to_present_ {};
};

class RenderBufferSource {
   public:
    explicit RenderBufferSource() = default;
    explicit RenderBufferSource(std::shared_ptr<ConcurrentBuffer> buffer);

    ~RenderBufferSource() = default;
    RenderBufferSource(RenderBufferSource&&) = default;
    auto operator=(RenderBufferSource&&) -> RenderBufferSource& = default;
    // disallow copy - as only one thread should own the source
    RenderBufferSource(const RenderBufferSource&) = delete;
    auto operator=(const RenderBufferSource&) -> RenderBufferSource& = delete;

   public:
    [[nodiscard]] auto params() const -> SwapChainParams;
    /**
     * @brief: Update swap chain parameters.
     *
     * Parameters are used the next time a frame is filled.
     */
    auto update_params(const SwapChainParams& new_params) -> void;

    /**
     * @brief: Render to the buffer via the given function.
     *
     * Raises QueueShutdownException after shutdown is intiated.
     */
    template <std::invocable<const SwapChainParams&, frame_t&> Func>
    auto render_to_buffer(Func fun) -> void;

   private:
    std::shared_ptr<ConcurrentBuffer> buffer_ {nullptr};
};

class RenderBufferSink {
   public:
    explicit RenderBufferSink() = default;
    explicit RenderBufferSink(std::shared_ptr<ConcurrentBuffer> buffer);

    ~RenderBufferSink() = default;
    RenderBufferSink(RenderBufferSink&&) = default;
    auto operator=(RenderBufferSink&&) -> RenderBufferSink& = default;
    // disallow copy - as only one thread should own the sink
    RenderBufferSink(const RenderBufferSink&) = delete;
    auto operator=(const RenderBufferSink&) -> RenderBufferSink& = delete;

   public:
    /**
     * @brief: Draw the buffer via the given function.
     *
     * Raises QueueShutdownException after shutdown is intiated.
     */
    template <std::invocable<const SwapChainParams&, const frame_t&> Func>
        requires std::same_as<
                     std::invoke_result_t<Func, const SwapChainParams&, frame_t&>,
                     BufferDrawStatus>
    auto draw_buffer(Func func) -> BufferDrawStatus;

   private:
    std::shared_ptr<ConcurrentBuffer> buffer_ {nullptr};
};

class RenderBufferControl {
   public:
    explicit RenderBufferControl() = default;
    explicit RenderBufferControl(std::shared_ptr<ConcurrentBuffer> buffer);

    /**
     * @brief: Destroy the control block and initiate a shutdown.
     */
    ~RenderBufferControl();
    RenderBufferControl(RenderBufferControl&&) = default;
    auto operator=(RenderBufferControl&&) -> RenderBufferControl& = default;
    // disallow copy - as only one thread should own thecontrol
    RenderBufferControl(const RenderBufferControl&) = delete;
    auto operator=(const RenderBufferControl&) -> RenderBufferControl& = delete;

   public:
    /**
     * @brief: Shut down the render buffer.
     *
     * This causes an QueueShutdownException to be raised on all
     * source and sink operations. Allowing the threads to shut down.
     */
    auto shutdown() -> void;

   private:
    std::shared_ptr<ConcurrentBuffer> buffer_ {nullptr};
};

/**
 * @Brief: Thread-safe Render Buffers parts that can be shared accross threads.
 */
struct RenderBufferParts {
    RenderBufferSource source;
    RenderBufferSink sink;
    RenderBufferControl control;
};

[[nodiscard]] auto create_render_buffer_parts(
    int count = LS_RENDER_BUFFER_DEFAULT_QUEUE_SIZE) -> RenderBufferParts;

//
// Implementation
//

template <std::invocable<SwapChainParams&, frame_t&> Func>
auto concurrent_buffer::RenderBuffer::modify(Func func) {
    const auto _ [[maybe_unused]] = std::lock_guard(mutex_);
    return func(params_, data_);
}

template <std::invocable<const SwapChainParams&, frame_t&> Func>
auto ConcurrentBuffer::render_to_buffer(Func func) -> void {
    auto buffer = ready_to_fill_.pop();
    Expects(buffer);

    try {
        // change params before new frame is rendered
        buffer->modify([func = std::move(func), new_params = new_params_.get()](
                           SwapChainParams& params, frame_t& frame) {
            params = new_params;
            func(params, frame);
        });
    }

    catch (...) {
        ready_to_fill_.push(std::move(buffer));
        throw;
    }

    ready_to_present_.push(std::move(buffer));
}

template <std::invocable<const SwapChainParams&, const frame_t&> Func>
    requires std::same_as<std::invoke_result_t<Func, const SwapChainParams&, frame_t&>,
                          BufferDrawStatus>
auto ConcurrentBuffer::draw_buffer(Func func) -> BufferDrawStatus {
    auto buffer = ready_to_present_.pop();
    Expects(buffer);

    const auto status = [&]() -> BufferDrawStatus {
        try {
            return buffer->modify(std::move(func));
        }

        catch (...) {
            ready_to_present_.push_front(std::move(buffer));
            throw;
        }
    }();

    if (status == BufferDrawStatus::DrawingSucceded) {
        ready_to_fill_.push(std::move(buffer));
    } else {
        ready_to_present_.push_front(std::move(buffer));
    }

    return status;
}

template <std::invocable<const SwapChainParams&, frame_t&> Func>
auto RenderBufferSource::render_to_buffer(Func func) -> void {
    if (buffer_) {
        buffer_->render_to_buffer(std::move(func));
    }
}

template <std::invocable<const SwapChainParams&, const frame_t&> Func>
    requires std::same_as<std::invoke_result_t<Func, const SwapChainParams&, frame_t&>,
                          BufferDrawStatus>
auto RenderBufferSink::draw_buffer(Func func) -> BufferDrawStatus {
    if (buffer_) {
        return buffer_->draw_buffer(std::move(func));
    }
    return BufferDrawStatus::DrawingFailed;
}

}  // namespace logicsim
