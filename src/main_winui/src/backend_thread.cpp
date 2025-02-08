#include "pch.h"

#include "main_winui/src/backend_thread.h"

namespace logicsim {

//
// Tasks
//

BackendTaskSink::BackendTaskSink(SharedBackendTaskQueue task_queue)
    : queue_ {std::move(task_queue)} {}

auto BackendTaskSink::pop() -> BackendTask {
    return queue_->pop();
}

auto BackendTaskSink::try_pop() -> std::optional<BackendTask> {
    return queue_->try_pop();
}

BackendTaskSource::~BackendTaskSource() {
    queue_->shutdown();
}

auto BackendTaskSource::get_sink() const -> BackendTaskSink {
    return BackendTaskSink {queue_};
}

auto BackendTaskSource::push(const BackendTask& task) -> void {
    queue_->push(task);
}

auto BackendTaskSource::push(BackendTask&& task) -> void {
    queue_->push(std::move(task));
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

auto handle_backend_task(const BackendTask& task, RenderBufferSource& render_source,
                         exporting::CircuitInterface& circuit) -> bool {
    using namespace exporting;

    if (const auto* item = std::get_if<MousePressEvent>(&task)) {
        circuit.mouse_press(*item);
        return true;
    }
    if (const auto* item = std::get_if<MouseMoveEvent>(&task)) {
        circuit.mouse_move(*item);
        return true;
    }
    if (const auto* item = std::get_if<MouseReleaseEvent>(&task)) {
        circuit.mouse_release(*item);
        return true;
    }
    if (const auto* item = std::get_if<MouseWheelEvent>(&task)) {
        circuit.mouse_wheel(*item);
        return true;
    }

    if (const auto* item = std::get_if<SwapChainParams>(&task)) {
        if (render_source.params() != *item) {
            render_source.update_params(*item);
            return true;
        }
        return false;
    }

    return false;
}

auto main_forwarded_tasks(std::stop_token& token, BackendTaskSink& tasks,
                          RenderBufferSource& render_source,
                          exporting::CircuitInterface& circuit) {
    while (!token.stop_requested()) {
        auto redraw = handle_backend_task(tasks.pop(), render_source, circuit);
        while (const auto task = tasks.try_pop()) {
            redraw |= handle_backend_task(*task, render_source, circuit);
        }

        if (redraw) {
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
        circuit.load(exporting::ExampleCircuitType::example_circuit_2);

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

auto create_backend_thread(std::unique_ptr<IBackendGuiActions> actions,
                           const BackendTaskSource& source,
                           RenderBufferSource render_source) -> std::jthread {
    return create_backend_thread(std::move(actions), source.get_sink(),
                                 std::move(render_source));
}

}  // namespace logicsim