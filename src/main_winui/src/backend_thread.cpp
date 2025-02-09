#include "pch.h"

#include "main_winui/src/backend_thread.h"

namespace logicsim {

//
// Tasks
//

BackendTaskSink::BackendTaskSink(SharedBackendTaskQueue task_queue)
    : queue_ {std::move(task_queue)} {}

auto BackendTaskSink::pop() -> BackendTask {
    Expects(queue_);
    return queue_->pop();
}

auto BackendTaskSink::try_pop() -> std::optional<BackendTask> {
    Expects(queue_);
    return queue_->try_pop();
}

BackendTaskSource::BackendTaskSource(SharedBackendTaskQueue task_queue)
    : queue_ {std::move(task_queue)} {
    Ensures(queue_);
}

BackendTaskSource::~BackendTaskSource() {
    if (queue_) {
        queue_->shutdown();
    }
}

auto BackendTaskSource::push(const BackendTask& task) -> void {
    Expects(queue_);
    queue_->push(task);
}

auto BackendTaskSource::push(BackendTask&& task) -> void {
    Expects(queue_);
    queue_->push(std::move(task));
}

auto create_backend_task_queue_parts() -> BackendTaskParts {
    auto queue = std::make_shared<BackendTaskQueue>();

    return BackendTaskParts {
        .source = BackendTaskSource {queue},
        .sink = BackendTaskSink {queue},
    };
}

namespace {

auto resize_buffer_discarding(const SwapChainParams& params, frame_t& frame) -> bool {
    const auto required_size = frame_buffer_size(params);
    const auto do_create = frame.size() != required_size;

    if (do_create) {
        frame = frame_t(required_size, frame_t::value_type {});
    }

    Ensures(frame_buffer_size(params) == frame.size());
    return do_create;
}

auto render_circuit(RenderBufferSource& render_source,
                    exporting::CircuitInterface& circuit) -> void {
    render_source.render_to_buffer([&](const SwapChainParams& params, frame_t& frame) {
        resize_buffer_discarding(params, frame);

        const auto width = params.width_pixel();
        const auto height = params.height_pixel();
        const auto pixel_ratio = params.rasterization_scale();

        auto* pixel_data = frame.data();
        const auto stride = intptr_t {width * LS_CANVAS_COLOR_BYTES};
        circuit.render_layout(width, height, pixel_ratio, pixel_data, stride);
    });
}

[[nodiscard]] auto handle_backend_task(const BackendTask& task,
                                       RenderBufferSource& render_source,
                                       exporting::CircuitInterface& circuit)
    -> ls_ui_status {
    using namespace exporting;

    if (const auto* item = std::get_if<MousePressEvent>(&task)) {
        return circuit.mouse_press(*item);
    }
    if (const auto* item = std::get_if<MouseMoveEvent>(&task)) {
        return circuit.mouse_move(*item);
    }
    if (const auto* item = std::get_if<MouseReleaseEvent>(&task)) {
        return circuit.mouse_release(*item);
    }
    if (const auto* item = std::get_if<MouseWheelEvent>(&task)) {
        return circuit.mouse_wheel(*item);
    }
    if (const auto* item = std::get_if<VirtualKey>(&task)) {
        return circuit.key_press(*item);
    }
    if (const auto* item = std::get_if<ExampleCircuitType>(&task)) {
        return circuit.load(*item);
    }

    if (const auto* item = std::get_if<SwapChainParams>(&task)) {
        if (render_source.params() != *item) {
            render_source.update_params(*item);
            return ls_ui_status {.repaint_required = true};
        }
        return ls_ui_status {};
    }

    return ls_ui_status {};
}

auto main_forwarded_tasks(std::stop_token& token, BackendTaskSink& tasks,
                          RenderBufferSource& render_source,
                          exporting::CircuitInterface& circuit) {
    while (!token.stop_requested()) {
        auto status = handle_backend_task(tasks.pop(), render_source, circuit);
        while (const auto task = tasks.try_pop()) {
            status |= handle_backend_task(*task, render_source, circuit);
        }

        if (status.repaint_required) {
            render_circuit(render_source, circuit);
        }
    }
}

auto backend_thread_main(std::stop_token token,
                         std::unique_ptr<IBackendGuiActions> actions,
                         BackendTaskSink tasks, RenderBufferSource render_source) {
    try {
        Expects(actions);
        winrt::init_apartment();

        auto circuit = exporting::CircuitInterface {};

        try {
            main_forwarded_tasks(token, tasks, render_source, circuit);
        } catch (const ShutdownException&) {
            // normal shutdown behavior.
        }

    } catch (const winrt::hresult_error& exc [[maybe_unused]]) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! {}\n\n");
    } catch (const std::exception& exc [[maybe_unused]]) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! {}\n\n");
    } catch (...) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! {}\n\n");
    }
}

}  // namespace

auto create_backend_thread(std::unique_ptr<IBackendGuiActions> actions,
                           BackendTaskSink sink, RenderBufferSource render_source)
    -> std::jthread {
    Expects(actions);

    return std::jthread(backend_thread_main, std::move(actions), std::move(sink),
                        std::move(render_source));
}

}  // namespace logicsim
