#include "pch.h"

#include "main_winui/src/backend_thread.h"

#include "main_winui/src/ls_overload.h"
#include "main_winui/src/ls_timer.h"

#include <gsl/gsl>

#include <memory>
#include <print>
#include <variant>

namespace logicsim {

//
// Tasks
//

BackendTaskSink::BackendTaskSink(SharedBackendTaskQueue task_queue)
    : queue_ {std::move(task_queue)} {
    Ensures(queue_);
}

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

[[nodiscard]] auto handle_circuit_ui_config_event(const CircuitUIConfigEvent& event,
                                                  exporting::CircuitInterface& circuit)
    -> UIStatus {
    const auto config = circuit.config();
    const auto new_config = exporting::CircuitUIConfig {
        .simulation =
            {
                .simulation_time_rate = event.simulation.simulation_time_rate.value_or(
                    config.simulation.simulation_time_rate),
                .use_wire_delay = event.simulation.use_wire_delay.value_or(
                    config.simulation.use_wire_delay),
            },
        .render =
            {
                .thread_count =
                    event.render.thread_count.value_or(config.render.thread_count),
                .wire_render_style = event.render.wire_render_style.value_or(
                    config.render.wire_render_style),
                //
                .do_benchmark =
                    event.render.do_benchmark.value_or(config.render.do_benchmark),
                .show_circuit =
                    event.render.show_circuit.value_or(config.render.show_circuit),
                .show_collision_index = event.render.show_collision_index.value_or(
                    config.render.show_collision_index),
                .show_connection_index = event.render.show_connection_index.value_or(
                    config.render.show_connection_index),
                .show_selection_index = event.render.show_selection_index.value_or(
                    config.render.show_selection_index),
                //
                .show_render_borders = event.render.show_render_borders.value_or(
                    config.render.show_render_borders),
                .show_mouse_position = event.render.show_mouse_position.value_or(
                    config.render.show_mouse_position),
                .direct_rendering = event.render.direct_rendering.value_or(
                    config.render.direct_rendering),
                .jit_rendering =
                    event.render.jit_rendering.value_or(config.render.jit_rendering),
            },
        .state =
            exporting::CircuitWidgetState {
                .type = event.state.type.value_or(config.state.type),
                .editing_default_mouse_action =
                    event.state.editing_default_mouse_action.value_or(
                        config.state.editing_default_mouse_action),
            },
    };

    if (new_config != config) {
        return circuit.set_config(new_config);
    }
    return UIStatus {};
}

[[nodiscard]] auto to_file_action(FileRequestEvent event) -> exporting::FileAction {
    using namespace exporting;
    using enum FileRequestEvent;

    switch (event) {
        case new_file:
            return FileAction::new_file;
        case open_file:
            return FileAction::open_file;
        case save_file:
            return FileAction::save_file;
        case save_as_file:
            return FileAction::save_as_file;

        case load_example_simple:
            return FileAction::load_example_simple;
        case load_example_elements_wires:
            return FileAction::load_example_elements_wires;
        case load_example_elements:
            return FileAction::load_example_elements;
        case load_example_wires:
            return FileAction::load_example_wires;

        case exit_application:
            return FileAction::new_file;
    };

    std::terminate();
}

[[nodiscard]] auto handle_file_request(FileRequestEvent event,
                                       exporting::CircuitInterface& circuit,
                                       IBackendGuiActions& actions) -> UIStatus {
    using namespace exporting;

    auto status = UIStatus {};
    auto next_step = std::optional<NextActionStep> {};

    status |= circuit.file_action(to_file_action(event), next_step);

    while (next_step.has_value()) {
        std::visit(overload(
                       [&](const ModalRequest& request) {
                           const auto response = actions.show_dialog_blocking(request);
                           status |= circuit.submit_modal_result(response, next_step);
                       },
                       [&](const ErrorMessage& message) {
                           actions.show_dialog_blocking(message);
                           next_step = std::nullopt;
                       }),
                   next_step.value());
    }

    actions.end_modal_state();

    return status;
}

[[nodiscard]] auto submit_backend_task(const BackendTask& task,
                                       RenderBufferSource& render_source,
                                       exporting::CircuitInterface& circuit,
                                       IBackendGuiActions& actions) -> UIStatus {
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
    if (const auto* item = std::get_if<UserActionEvent>(&task)) {
        return circuit.do_action(*item);
    }
    if (const auto* item = std::get_if<CircuitUIConfigEvent>(&task)) {
        return handle_circuit_ui_config_event(*item, circuit);
    }
    if (const auto* item = std::get_if<FileRequestEvent>(&task)) {
        return handle_file_request(*item, circuit, actions);
    }

    if (const auto* item = std::get_if<SwapChainParams>(&task)) {
        if (render_source.params() != *item) {
            render_source.update_params(*item);
            return UIStatus {.repaint_required = true};
        }
        return UIStatus {};
    }

    return UIStatus {};
}

auto process_backend_task(const BackendTask& task, RenderBufferSource& render_source,
                          exporting::CircuitInterface& circuit,
                          IBackendGuiActions& actions) -> void {
    const auto status = submit_backend_task(task, render_source, circuit, actions);

    if (status.config_changed) {
        actions.config_update(circuit.config());
    }
    if (status.filename_changed) {
        actions.change_title(winrt::hstring {circuit.display_filename().native()});
    }
    if (status.repaint_required) {
        render_circuit(render_source, circuit);
    }
}

template <typename T>
[[nodiscard]] auto both_hold_type(const std::optional<BackendTask>& a,
                                  const std::optional<BackendTask>& b) -> bool {
    return a && b && std::holds_alternative<T>(*a) && std::holds_alternative<T>(*b);
}

/**
 * @brief: Returns combined event for consecutive events.
 */
[[nodiscard]] auto combine_consecutive_tasks(const std::optional<BackendTask>& first,
                                             const std::optional<BackendTask>& second)
    -> std::optional<BackendTask> {
    using namespace exporting;

    if (both_hold_type<MouseMoveEvent>(first, second) ||
        both_hold_type<SwapChainParams>(first, second)) {
        return second;  // process newest event only
    }
    if (both_hold_type<MouseWheelEvent>(first, second)) {
        return combine_wheel_event(std::get<MouseWheelEvent>(*first),
                                   std::get<MouseWheelEvent>(*second));
    }

    // not combinable
    return std::nullopt;
}

auto main_forwarded_tasks(std::stop_token& token, BackendTaskSink& tasks,
                          RenderBufferSource& render_source,
                          exporting::CircuitInterface& circuit,
                          IBackendGuiActions& actions) {
    auto first = std::optional<BackendTask> {};

    while (!token.stop_requested()) {
        const auto is_benchmark = circuit.is_render_do_benchmark();

        if (!first) {
            if (is_benchmark) {
                first = tasks.try_pop();
            } else {
                first = tasks.pop();
            }
        }

        if (is_benchmark && !first) {
            render_circuit(render_source, circuit);
            continue;
        }

        Expects(first);
        auto second = tasks.try_pop();

        // try to combine
        if (const auto combined = combine_consecutive_tasks(first, second)) {
            first = combined;
            second = std::nullopt;
            continue;
        }

        process_backend_task(first.value(), render_source, circuit, actions);
        first = std::exchange(second, std::nullopt);
    }
}

auto backend_thread_main(std::stop_token token,
                         std::unique_ptr<IBackendGuiActions> actions,
                         BackendTaskSink tasks, RenderBufferSource render_source) {
    try {
        Expects(actions);
        winrt::init_apartment();

        auto circuit = exporting::CircuitInterface {};

        actions->config_update(circuit.config());
        actions->change_title(winrt::hstring {circuit.display_filename().native()});

        try {
            main_forwarded_tasks(token, tasks, render_source, circuit, *actions);
        } catch (const ShutdownException&) {
            // normal shutdown behavior.
        }

    } catch (const winrt::hresult_error& exc [[maybe_unused]]) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! {}\n\n");
        std::print("\n!!! CRASH EXCEPTION BACKEND-THREAD !!!!\n\n");
        std::terminate();
    } catch (const std::exception& exc [[maybe_unused]]) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! {}\n\n");
        std::print("\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! {}\n\n", exc.what());
        std::terminate();
    } catch (...) {
        OutputDebugStringW(L"\n!!! CRASH EXCEPTION BACKEND-THREAD !!!! {}\n\n");
        std::print("\n!!! CRASH EXCEPTION BACKEND-THREAD !!!!\n\n");
        std::terminate();
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
