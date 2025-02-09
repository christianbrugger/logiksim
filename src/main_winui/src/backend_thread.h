#pragma once

#include "main_winui/src/ls_concurrent_blocking_queue.h"
#include "main_winui/src/render_buffer.h"

#include "core_export/logicsim_core_export.h"

#include <gsl/gsl>

#include <windows.h>
#include <winrt/Microsoft.Graphics.Canvas.h>
#include <winrt/Microsoft.UI.Input.h>

#include <memory>
#include <optional>
#include <thread>
#include <variant>

namespace logicsim {

//
// Actions
//

/**
 * @brief: Communication from backend thread to UI.
 *
 * Class-invariants:
 *   + All methods can be safely called from non-UI threads.
 *   + Interface only type, all functions are pure virtual (C.121)
 */
class IBackendGuiActions {
   public:
    explicit IBackendGuiActions() = default;
    // polymorphic types require virtual destructor (C.127)
    virtual ~IBackendGuiActions() = default;
    // delete copy & move to prevent slicing on polymorphic classes (C.67)
    IBackendGuiActions(const IBackendGuiActions&) = delete;
    IBackendGuiActions(IBackendGuiActions&&) = delete;
    auto operator=(const IBackendGuiActions&) -> IBackendGuiActions& = delete;
    auto operator=(IBackendGuiActions&&) -> IBackendGuiActions& = delete;

   public:
    virtual auto change_title(winrt::hstring title) const -> void = 0;
};

//
// Tasks
//

using BackendTask = std::variant<            //
    SwapChainParams,                         //
    logicsim::exporting::MousePressEvent,    //
    logicsim::exporting::MouseMoveEvent,     //
    logicsim::exporting::MouseReleaseEvent,  //
    logicsim::exporting::MouseWheelEvent,    //
    logicsim::exporting::VirtualKey          //
    >;

using BackendTaskQueue = ::logicsim::ConcurrentBlockingQueue<BackendTask>;

using SharedBackendTaskQueue = gsl::not_null<std::shared_ptr<BackendTaskQueue>>;

class BackendTaskSink {
   public:
    BackendTaskSink() = default;
    explicit BackendTaskSink(SharedBackendTaskQueue task_queue);

    auto pop() -> BackendTask;
    auto try_pop() -> std::optional<BackendTask>;

   private:
    SharedBackendTaskQueue queue_ {std::make_shared<BackendTaskQueue>()};
};

class BackendTaskSource {
   public:
    BackendTaskSource() = default;
    /**
     * @brief: Destroy the task source and initiate a shutdown.
     */
    ~BackendTaskSource();
    BackendTaskSource(BackendTaskSource&&) = default;
    auto operator=(BackendTaskSource&&) -> BackendTaskSource& = default;
    // disallow copy - as destruction initiates shutdown
    BackendTaskSource(const BackendTaskSource&) = delete;
    auto operator=(const BackendTaskSource&) -> BackendTaskSource& = delete;

   public:
    auto get_sink() const -> BackendTaskSink;

    auto push(const BackendTask& task) -> void;
    auto push(BackendTask&& task) -> void;

   private:
    SharedBackendTaskQueue queue_ {std::make_shared<BackendTaskQueue>()};
};

//
// Thread
//

[[nodiscard]] auto create_backend_thread(std::unique_ptr<IBackendGuiActions> actions,
                                         BackendTaskSink sink,
                                         RenderBufferSource render_source)
    -> std::jthread;

[[nodiscard]] auto create_backend_thread(std::unique_ptr<IBackendGuiActions> actions,
                                         const BackendTaskSource& source,
                                         RenderBufferSource render_source)
    -> std::jthread;

}  // namespace logicsim