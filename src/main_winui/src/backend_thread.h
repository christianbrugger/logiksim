#pragma once

#include "main_winui/src/ls_concurrent_blocking_queue.h"
#include "main_winui/src/render_buffer.h"

#include "core_export/logicsim_core_export.h"

#include <winrt/base.h>

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
    virtual auto config_update(logicsim::exporting::CircuitUIConfig config) const
        -> void = 0;

    [[nodiscard]] virtual auto show_dialog_blocking(exporting::ModalRequest request) const
        -> exporting::ModalResult = 0;
    virtual auto show_dialog_blocking(exporting::ErrorMessage message) const -> void = 0;
    virtual auto end_modal_state() const -> void = 0;
    virtual auto exit_application_no_dialog() const -> void = 0;
};

//
// Tasks
//

struct SimulationConfigEvent {
    std::optional<exporting::time_rate_t> simulation_time_rate {};
    std::optional<bool> use_wire_delay {};

    [[nodiscard]] auto operator==(const SimulationConfigEvent& other) const
        -> bool = default;
};

struct WidgetRenderConfigEvent {
    std::optional<exporting::ThreadCount> thread_count {};
    std::optional<exporting::WireRenderStyle> wire_render_style {};

    std::optional<bool> do_benchmark {};
    std::optional<bool> show_circuit {};
    std::optional<bool> show_collision_index {};
    std::optional<bool> show_connection_index {};
    std::optional<bool> show_selection_index {};

    std::optional<bool> show_render_borders {};
    std::optional<bool> show_mouse_position {};
    std::optional<bool> direct_rendering {};
    std::optional<bool> jit_rendering {};

    [[nodiscard]] auto operator==(const WidgetRenderConfigEvent&) const -> bool = default;
};

struct CircuitWidgetStateEvent {
    std::optional<exporting::CircuitStateType> type {};
    std::optional<exporting::DefaultMouseAction> editing_default_mouse_action {};

    [[nodiscard]] auto operator==(const CircuitWidgetStateEvent&) const -> bool = default;
};

struct CircuitUIConfigEvent {
    SimulationConfigEvent simulation {};
    WidgetRenderConfigEvent render {};
    CircuitWidgetStateEvent state {};

    [[nodiscard]] auto operator==(const CircuitUIConfigEvent&) const -> bool = default;
};

struct OpenFileNonModalEvent {
    std::filesystem::path filename {};

    [[nodiscard]] auto operator==(const OpenFileNonModalEvent&) const -> bool = default;
};

using BackendTask = std::variant<  //
    SwapChainParams,               //
    exporting::MousePressEvent,    //
    exporting::MouseMoveEvent,     //
    exporting::MouseReleaseEvent,  //
    exporting::MouseWheelEvent,    //
    exporting::VirtualKey,         //
    exporting::UserActionEvent,    //
    exporting::FileAction,         //
    CircuitUIConfigEvent,          //
    OpenFileNonModalEvent          //
    >;

using BackendTaskQueue = ::logicsim::ConcurrentBlockingQueue<BackendTask>;

using SharedBackendTaskQueue = std::shared_ptr<BackendTaskQueue>;

class BackendTaskSink {
   public:
    BackendTaskSink() = default;
    explicit BackendTaskSink(SharedBackendTaskQueue task_queue);

    ~BackendTaskSink() = default;
    BackendTaskSink(BackendTaskSink&&) = default;
    auto operator=(BackendTaskSink&&) -> BackendTaskSink& = default;
    // disallow copy - as only one thread should own the sink
    BackendTaskSink(const BackendTaskSink&) = delete;
    auto operator=(const BackendTaskSink&) -> BackendTaskSink& = delete;

   public:
    auto pop() -> BackendTask;
    auto try_pop() -> std::optional<BackendTask>;

   private:
    SharedBackendTaskQueue queue_ {nullptr};
};

class BackendTaskSource {
   public:
    BackendTaskSource() = default;
    explicit BackendTaskSource(SharedBackendTaskQueue task_queue);
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
    auto push(const BackendTask& task) -> void;
    auto push(BackendTask&& task) -> void;

   private:
    SharedBackendTaskQueue queue_ {nullptr};
};

/**
 * @Brief: Thread-safe Backend Task Queue parts that can be shared accross threads.
 */
struct BackendTaskParts {
    BackendTaskSource source;
    BackendTaskSink sink;
};

[[nodiscard]] auto create_backend_task_queue_parts() -> BackendTaskParts;

//
// Thread
//

[[nodiscard]] auto create_backend_thread(std::unique_ptr<IBackendGuiActions> actions,
                                         BackendTaskSink sink,
                                         RenderBufferSource render_source)
    -> std::jthread;

}  // namespace logicsim