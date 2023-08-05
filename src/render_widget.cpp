#include "render_widget.h"

#include "collision.h"
#include "exceptions.h"
#include "layout.h"
#include "range.h"
#include "schematic.h"
#include "schematic_generation.h"
#include "serialize.h"
#include "simulation.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>

namespace logicsim {

template <>
auto format(InteractionState state) -> std::string {
    switch (state) {
        using enum InteractionState;

        case not_interactive:
            return "not_interactive";
        case selection:
            return "selection";
        case simulation:
            return "simulation";

        case insert_wire:
            return "insert_wire";
        case insert_button:
            return "insert_button";

        case insert_and_element:
            return "insert_and_element";
        case insert_or_element:
            return "insert_or_element";
        case insert_xor_element:
            return "insert_xor_element";
        case insert_nand_element:
            return "insert_nand_element";
        case insert_nor_element:
            return "insert_nor_element";

        case insert_buffer_element:
            return "insert_buffer_element";
        case insert_inverter_element:
            return "insert_inverter_element";
        case insert_flipflop_jk:
            return "insert_flipflop_jk";
        case insert_latch_d:
            return "insert_latch_d";
        case insert_flipflop_d:
            return "insert_flipflop_d";
        case insert_flipflop_ms_d:
            return "insert_flipflop_ms_d";

        case insert_clock_generator:
            return "insert_clock_generator";
        case insert_shift_register:
            return "insert_shift_register";
    }
    throw_exception("Don't know how to convert InteractionState to string.");
}

auto is_inserting_state(InteractionState state) -> bool {
    using enum InteractionState;
    return state != not_interactive && state != selection && state != simulation;
}

auto to_logic_item_definition(InteractionState state, std::size_t default_input_count)
    -> LogicItemDefinition {
    switch (state) {
        using enum InteractionState;

        case not_interactive:
        case selection:
        case simulation:
            throw_exception("non-inserting states don't have a definition");

        case insert_wire:
            return LogicItemDefinition {
                .element_type = ElementType::wire,
                .input_count = 0,
                .orientation = orientation_t::undirected,
            };
        case insert_button:
            return LogicItemDefinition {
                .element_type = ElementType::button,
                .input_count = 0,
                .orientation = orientation_t::undirected,
            };

        case insert_and_element:
            return LogicItemDefinition {
                .element_type = ElementType::and_element,
                .input_count = default_input_count,
                .orientation = orientation_t::right,
            };
        case insert_or_element:
            return LogicItemDefinition {
                .element_type = ElementType::or_element,
                .input_count = default_input_count,
                .orientation = orientation_t::right,
            };
        case insert_xor_element:
            return LogicItemDefinition {
                .element_type = ElementType::xor_element,
                .input_count = default_input_count,
                .orientation = orientation_t::right,
            };
        case insert_nand_element:
            return LogicItemDefinition {
                .element_type = ElementType::and_element,
                .input_count = default_input_count,
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };
        case insert_nor_element:
            return LogicItemDefinition {
                .element_type = ElementType::or_element,
                .input_count = default_input_count,
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };

        case insert_buffer_element:
            return LogicItemDefinition {
                .element_type = ElementType::buffer_element,
                .input_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_inverter_element:
            return LogicItemDefinition {
                .element_type = ElementType::buffer_element,
                .input_count = 1,
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };

        case insert_flipflop_jk:
            return LogicItemDefinition {
                .element_type = ElementType::flipflop_jk,
                .input_count = 5,
                .output_count = 2,
                .orientation = orientation_t::right,
            };
        case insert_latch_d:
            return LogicItemDefinition {
                .element_type = ElementType::latch_d,
                .input_count = 2,
                .output_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_flipflop_d:
            return LogicItemDefinition {
                .element_type = ElementType::flipflop_d,
                .input_count = 4,
                .output_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_flipflop_ms_d:
            return LogicItemDefinition {
                .element_type = ElementType::flipflop_ms_d,
                .input_count = 4,
                .output_count = 1,
                .orientation = orientation_t::right,
            };

        case insert_clock_generator:
            return LogicItemDefinition {
                .element_type = ElementType::clock_generator,
                .input_count = 2,
                .output_count = 2,
                .orientation = orientation_t::right,
            };
        case insert_shift_register:
            return LogicItemDefinition {
                .element_type = ElementType::shift_register,
                .input_count = 3,
                .output_count = 2,
                .orientation = orientation_t::right,
            };
    }
    throw_exception("Don't know how to convert InteractionState to definition.");
}

//
// Mouse Drag Logic
//

MouseDragLogic::MouseDragLogic(Args args) noexcept : config_ {args.view_config} {}

auto MouseDragLogic::mouse_press(QPointF position) -> void {
    last_position = position;
}

auto MouseDragLogic::mouse_move(QPointF position) -> void {
    if (last_position.has_value()) {
        const auto new_offset = config_.offset()                   //
                                + to_grid_fine(position, config_)  //
                                - to_grid_fine(*last_position, config_);
        config_.set_offset(new_offset);
        last_position = position;
    }
}

auto MouseDragLogic::mouse_release(QPointF position) -> void {
    mouse_move(position);
    last_position = std::nullopt;
}

//
// Mouse Insert Logic
//

MouseElementInsertLogic::MouseElementInsertLogic(Args args) noexcept
    : editable_circuit_ {args.editable_circuit},
      element_definition_ {args.element_definition} {}

MouseElementInsertLogic::~MouseElementInsertLogic() {
    remove_last_element();
}

auto MouseElementInsertLogic::mouse_press(std::optional<point_t> position) -> void {
    remove_and_insert(position, InsertionMode::collisions);
}

auto MouseElementInsertLogic::mouse_move(std::optional<point_t> position) -> void {
    remove_and_insert(position, InsertionMode::collisions);
}

auto MouseElementInsertLogic::mouse_release(std::optional<point_t> position) -> void {
    remove_and_insert(position, InsertionMode::insert_or_discard);
    temp_element_.reset();
}

auto MouseElementInsertLogic::remove_last_element() -> void {
    if (temp_element_) {
        editable_circuit_.delete_all(std::move(temp_element_));
    }
}

auto MouseElementInsertLogic::remove_and_insert(std::optional<point_t> position,
                                                InsertionMode mode) -> void {
    remove_last_element();
    assert(!temp_element_);

    if (position) {
        temp_element_ = editable_circuit_.add_logic_item(element_definition_,
                                                         position.value(), mode);
    }
}

//
// Mouse Line Insert Logic
//

MouseLineInsertLogic::MouseLineInsertLogic(Args args) noexcept
    : editable_circuit_ {args.editable_circuit} {}

MouseLineInsertLogic::~MouseLineInsertLogic() {
    remove_last_element();
}

auto MouseLineInsertLogic::mouse_press(std::optional<point_t> position) -> void {
    first_position_ = position;
    remove_and_insert(position, InsertionMode::collisions);
}

auto MouseLineInsertLogic::mouse_move(std::optional<point_t> position) -> void {
    remove_and_insert(position, InsertionMode::collisions);
}

auto MouseLineInsertLogic::mouse_release(std::optional<point_t> position) -> void {
    if (position && first_position_ && position == first_position_) {
        editable_circuit_.toggle_inverter(*position);
    }

    remove_and_insert(position, InsertionMode::insert_or_discard);
    temp_element_.reset();
}

auto MouseLineInsertLogic::remove_last_element() -> void {
    if (temp_element_) {
        editable_circuit_.delete_all(std::move(temp_element_));
    }
}

auto MouseLineInsertLogic::remove_and_insert(std::optional<point_t> position,
                                             InsertionMode mode) -> void {
    remove_last_element();
    assert(!temp_element_);

    if (position && first_position_) {
        temp_element_ = editable_circuit_.add_line_segments(
            *first_position_, *position, LineSegmentType::horizontal_first, mode);
    }
}

//
// Mouse Move Selection Logic
//

MouseMoveSelectionLogic::MouseMoveSelectionLogic(Args args)
    : builder_ {args.builder},
      editable_circuit_ {args.editable_circuit},
      delete_on_cancel_ {args.delete_on_cancel},
      split_points_ {std::move(args.split_points)} {
    if (args.has_colliding) {
        state_ = State::waiting_for_confirmation;
        insertion_mode_ = InsertionMode::collisions;
    }
}

MouseMoveSelectionLogic::~MouseMoveSelectionLogic() {
    if (!finished()) {
        if (delete_on_cancel_) {
            delete_selection();
        } else {
            restore_original_positions();
        }
    }
    convert_to(InsertionMode::insert_or_discard);

    if (state_ == State::finished_confirmed) {
        builder_.clear();
    }
}

namespace {

auto all_selected(const Selection& selection, const Layout& layout,
                  std::span<const SpatialTree::query_result_t> items, point_fine_t point)
    -> bool {
    for (const auto& item : items) {
        if (!item.segment_index) {
            if (!selection.is_selected(item.element_id)) {
                return false;
            }
        } else {
            const auto segment = segment_t {item.element_id, item.segment_index};
            if (!is_selected(selection, layout, segment, point)) {
                return false;
            }
        }
    }
    return true;
}

auto anything_selected(const Selection& selection, const Layout& layout,
                       std::span<const SpatialTree::query_result_t> items,
                       point_fine_t point) -> bool {
    for (const auto& item : items) {
        if (!item.segment_index) {
            if (selection.is_selected(item.element_id)) {
                return true;
            }
        } else {
            const auto segment = segment_t {item.element_id, item.segment_index};
            if (is_selected(selection, layout, segment, point)) {
                return true;
            }
        }
    }
    return false;
}

auto add_to_selection(Selection& selection, const Layout& layout,
                      std::span<const SpatialTree::query_result_t> items, bool whole_tree)
    -> void {
    for (const auto& item : items) {
        if (!item.segment_index) {
            selection.add_logicitem(item.element_id);
        } else {
            if (whole_tree) {
                add_segment_tree(selection, item.element_id, layout);
            } else {
                const auto segment = segment_t {item.element_id, item.segment_index};
                add_segment(selection, segment, layout);
            }
        }
    }
}

}  // namespace

auto MouseMoveSelectionLogic::mouse_press(point_fine_t point, bool double_click) -> void {
    if (state_ == State::waiting_for_first_click) {
        const auto items = editable_circuit_.caches().spatial_cache().query_selection(
            rect_fine_t {point, point});

        if (items.empty()) {
            builder_.clear();
            state_ = State::finished;
            return;
        }

        if (!anything_selected(builder_.selection(), editable_circuit_.layout(), items,
                               point)) {
            auto selection = Selection {};
            add_to_selection(selection, editable_circuit_.layout(), items, false);
            builder_.set_selection(selection);
        }

        if (double_click) {
            auto selection = Selection {builder_.selection()};
            add_to_selection(selection, editable_circuit_.layout(), items, true);
            builder_.set_selection(selection);
        }
    }

    if (state_ == State::waiting_for_first_click
        || state_ == State::waiting_for_confirmation) {
        state_ = State::move_selection;
        last_position_ = point;
    }
}

auto MouseMoveSelectionLogic::move_selection(point_fine_t point) -> void {
    if (!last_position_) {
        return;
    }

    const auto delta_x = round_to<int>(point.x - last_position_->x);
    const auto delta_y = round_to<int>(point.y - last_position_->y);

    if (delta_x == 0 && delta_y == 0) {
        return;
    }

    const auto& selection = get_selection();

    if (!editable_circuit_.new_positions_representable(selection, delta_x, delta_y)) {
        return;
    }

    convert_to(InsertionMode::temporary);
    editable_circuit_.move_or_delete_elements(copy_selection(), delta_x, delta_y);
    if (split_points_) {
        split_points_ = move_or_delete_points(split_points_.value(), delta_x, delta_y);
    }

    last_position_ = point_fine_t {
        last_position_->x + delta_x,
        last_position_->y + delta_y,
    };
    total_offsets_.first += delta_x;
    total_offsets_.second += delta_y;
}

auto MouseMoveSelectionLogic::mouse_move(point_fine_t point) -> void {
    if (state_ != State::move_selection) {
        return;
    }
    move_selection(point);
}

auto MouseMoveSelectionLogic::mouse_release(point_fine_t point) -> void {
    if (state_ != State::move_selection) {
        return;
    }
    move_selection(point);

    convert_to(InsertionMode::collisions);
    const auto collisions = calculate_any_element_colliding();

    if (collisions) {
        state_ = State::waiting_for_confirmation;
    } else {
        state_ = State::finished;
    }
}

auto MouseMoveSelectionLogic::confirm() -> void {
    if (state_ != State::waiting_for_confirmation) {
        return;
    }

    state_ = State::finished_confirmed;
}

auto MouseMoveSelectionLogic::finished() -> bool {
    return state_ == State::finished || state_ == State::finished_confirmed;
}

auto MouseMoveSelectionLogic::get_selection() -> const Selection& {
    builder_.apply_all_operations();
    return builder_.selection();
}

auto MouseMoveSelectionLogic::copy_selection() -> selection_handle_t {
    return editable_circuit_.create_selection(get_selection());
}

auto MouseMoveSelectionLogic::convert_to(InsertionMode mode) -> void {
    if (insertion_mode_ == mode) {
        return;
    }
    if (mode == InsertionMode::temporary && !split_points_) {
        split_points_.emplace(
            editable_circuit_.capture_inserted_splitpoints(get_selection()));
    }

    insertion_mode_ = mode;
    editable_circuit_.change_insertion_mode(copy_selection(), mode);

    if (mode == InsertionMode::temporary && split_points_) {
        editable_circuit_.split_temporary_segments(split_points_.value(),
                                                   get_selection());
    }
    if (mode == InsertionMode::temporary) {
        editable_circuit_.add_temporary_crosspoints(get_selection());
    }
}

auto MouseMoveSelectionLogic::restore_original_positions() -> void {
    if (total_offsets_.first == 0 && total_offsets_.second == 0) {
        return;
    }

    convert_to(InsertionMode::temporary);
    editable_circuit_.move_or_delete_elements(copy_selection(), -total_offsets_.first,
                                              -total_offsets_.second);
}

auto MouseMoveSelectionLogic::calculate_any_element_colliding() -> bool {
    return anything_colliding(get_selection(), editable_circuit_.layout());
}

auto MouseMoveSelectionLogic::delete_selection() -> void {
    auto handle = copy_selection();
    builder_.clear();
    editable_circuit_.delete_all(std::move(handle));
}

//
// Mouse Item Selection Logic
//

MouseSingleSelectionLogic::MouseSingleSelectionLogic(Args args)
    : builder_ {args.builder}, editable_circuit_ {args.editable_circuit} {}

namespace {

auto add_selection(Selection& selection, const Layout& layout,
                   std::span<const SpatialTree::query_result_t> items, point_fine_t point)
    -> void {
    for (const auto& item : items) {
        if (!item.segment_index) {
            selection.add_logicitem(item.element_id);

        } else {
            const auto segment = segment_t {item.element_id, item.segment_index};
            add_segment_part(selection, layout, segment, point);
        }
    }
}

auto remove_selection(Selection& selection, const Layout& layout,
                      std::span<const SpatialTree::query_result_t> items,
                      point_fine_t point) -> void {
    for (const auto& item : items) {
        if (!item.segment_index) {
            selection.remove_logicitem(item.element_id);

        } else {
            const auto segment = segment_t {item.element_id, item.segment_index};
            remove_segment_part(selection, layout, segment, point);
        }
    }
}

auto add_whole_trees(Selection& selection, const Layout& layout,
                     std::span<const SpatialTree::query_result_t> items) -> void {
    for (const auto& item : items) {
        if (item.segment_index) {
            add_segment_tree(selection, item.element_id, layout);
        }
    }
}

auto remove_whole_trees(Selection& selection, const Layout& layout,
                        std::span<const SpatialTree::query_result_t> items) -> void {
    for (const auto& item : items) {
        if (item.segment_index) {
            remove_segment_tree(selection, item.element_id, layout);
        }
    }
}

}  // namespace

auto MouseSingleSelectionLogic::mouse_press(point_fine_t point, bool double_click)
    -> void {
    // builder_.add(SelectionFunction::toggle, rect_fine_t {point, point});
    const auto& layout = editable_circuit_.layout();

    const auto items = editable_circuit_.caches().spatial_cache().query_selection(
        rect_fine_t {point, point});

    if (items.empty()) {
        return;
    }

    auto selection = Selection {builder_.selection()};

    if (!double_click) {
        if (!all_selected(selection, layout, items, point)) {
            add_selection(selection, layout, items, point);
        } else {
            remove_selection(selection, layout, items, point);
        }
    } else {
        if (!all_selected(selection, layout, items, point)) {
            remove_whole_trees(selection, layout, items);
        } else {
            add_whole_trees(selection, layout, items);
        }
    }

    builder_.set_selection(selection);
}

auto MouseSingleSelectionLogic::mouse_move(point_fine_t point [[maybe_unused]]) -> void {}

auto MouseSingleSelectionLogic::mouse_release(point_fine_t point [[maybe_unused]])
    -> void {}

//
// Mouse Area Selection Logic
//

MouseAreaSelectionLogic::MouseAreaSelectionLogic(Args args)
    : builder_ {args.builder},
      view_config_ {args.view_config},
      band_ {QRubberBand::Rectangle, args.parent} {}

MouseAreaSelectionLogic::~MouseAreaSelectionLogic() {
    if (!keep_last_selection_) {
        builder_.pop_last();
    }
}

auto MouseAreaSelectionLogic::mouse_press(QPointF position,
                                          Qt::KeyboardModifiers modifiers) -> void {
    const auto p0 = to_grid_fine(position, view_config_);

    const auto function = [modifiers] {
        if (modifiers == Qt::AltModifier) {
            return SelectionFunction::substract;
        }
        return SelectionFunction::add;
    }();

    if (modifiers == Qt::NoModifier) {
        builder_.clear();
    }

    builder_.add(function, rect_fine_t {p0, p0});
    first_position_ = p0;
}

auto MouseAreaSelectionLogic::mouse_move(QPointF position) -> void {
    update_mouse_position(position);
}

auto MouseAreaSelectionLogic::mouse_release(QPointF position) -> void {
    update_mouse_position(position);
    keep_last_selection_ = true;
}

auto MouseAreaSelectionLogic::update_mouse_position(QPointF position) -> void {
    if (!first_position_) {
        return;
    }

    // order points
    const auto q0 = to_widget(*first_position_, view_config_);
    const auto q1 = position.toPoint();
    const auto [x0, x1] = sorted(q0.x(), q1.x());
    const auto [y0, y1] = sorted(q0.y(), q1.y());

    // QRect
    const auto q_minimum = QPoint {x0, y0};
    const auto q_maximum = QPoint {x1, y1};
    const auto q_rect = QRect {q_minimum, q_maximum};

    // rect_fine_t
    const auto a_minimum = to_grid_fine(q_minimum, view_config_);
    const auto a_maximum = to_grid_fine(q_maximum, view_config_);
    const auto grid_rect = rect_fine_t {a_minimum, a_maximum};

    // visualize rect
    band_.setGeometry(q_rect);
    band_.show();

    builder_.update_last(grid_rect);
}

//
// Simulation Interaction Logic
//

SimulationInteractionLogic::SimulationInteractionLogic(Args args)
    : simulation_ {args.simulation} {}

SimulationInteractionLogic::~SimulationInteractionLogic() {}

auto SimulationInteractionLogic::mouse_press(std::optional<point_t> point) -> void {
    if (point) {
        simulation_.mouse_press(point.value());
    }
}

//
// Render Widget
//

RendererWidget::RendererWidget(QWidget* parent)
    : QWidget(parent),
      last_pixel_ratio_ {devicePixelRatioF()},
      mouse_drag_logic_ {MouseDragLogic::Args {render_settings_.view_config}} {
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    // accept focus so keyboard signals gets fired
    setFocusPolicy(Qt::StrongFocus);

    connect(&benchmark_timer_, &QTimer::timeout, this,
            &RendererWidget::on_benchmark_timeout);
    connect(&simulation_timer_, &QTimer::timeout, this,
            &RendererWidget::on_simulation_timeout);
    simulation_timer_.setInterval(simulation_timer_interval_ms_);

    render_settings_.view_config.set_device_scale(18);
    reset_circuit();
}

auto RendererWidget::set_do_benchmark(bool value) -> void {
    do_benchmark_ = value;

    if (value) {
        benchmark_timer_.start();
    } else {
        benchmark_timer_.stop();
    }

    update();
}

auto RendererWidget::set_do_render_circuit(bool value) -> void {
    do_render_circuit_ = value;
    update();
}

auto RendererWidget::set_do_render_collision_cache(bool value) -> void {
    do_render_collision_cache_ = value;
    update();
}

auto RendererWidget::set_do_render_connection_cache(bool value) -> void {
    do_render_connection_cache_ = value;
    update();
}

auto RendererWidget::set_do_render_selection_cache(bool value) -> void {
    do_render_selection_cache_ = value;
    update();
}

auto RendererWidget::set_interaction_state(InteractionState state) -> void {
    if (interaction_state_ != state) {
        interaction_state_ = state;
        reset_interaction_state();
    }
    emit interaction_state_changed(state);

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

// void RendererWidget::interaction_state_changed(InteractionState new_state) {}

auto RendererWidget::set_default_input_count(std::size_t count) -> void {
    default_input_count_ = count;
}

auto RendererWidget::set_time_rate(time_rate_t time_rate) -> void {
    time_rate_ = time_rate;

    if (simulation_) {
        simulation_->set_time_rate(time_rate);
    }
}

auto RendererWidget::set_wire_delay_per_distance(delay_t value) -> void {
    if (interaction_state_ == InteractionState::simulation) [[unlikely]] {
        throw_exception("cannot set wire delay during active simulation");
    }
    wire_delay_per_distance_ = value;
}

auto RendererWidget::interaction_state() -> InteractionState {
    return interaction_state_;
}

auto RendererWidget::default_input_count() -> std::size_t {
    return default_input_count_;
}

auto RendererWidget::time_rate() const -> time_rate_t {
    return time_rate_;
}

auto RendererWidget::wire_delay_per_distance() const -> delay_t {
    return wire_delay_per_distance_;
}

auto RendererWidget::reset_interaction_state() -> void {
    mouse_logic_.reset();
    simulation_.reset();
    if (editable_circuit_) {
        editable_circuit_->selection_builder().clear();
    }

    if (interaction_state_ == InteractionState::simulation) {
        simulation_timer_.start();
    } else {
        simulation_timer_.stop();
    }

    update();
};

auto RendererWidget::fps() const -> double {
    return fps_counter_.events_per_second();
}

auto RendererWidget::pixel_scale() const -> double {
    return render_settings_.view_config.pixel_scale();
}

auto RendererWidget::reset_circuit() -> void {
    reset_interaction_state();

    circuit_index_ = CircuitIndex {};
    editable_circuit_.emplace(
        circuit_index_.borrow_circuit(circuit_id_).extract_layout());
    update();

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

auto RendererWidget::reload_circuit() -> void {
    if (!editable_circuit_) {
        return;
    }
    reset_interaction_state();

    {
        const auto t = Timer {"reload", Timer::Unit::ms, 3};

        auto layout = editable_circuit_->extract_layout();

        editable_circuit_.reset();
        editable_circuit_.emplace(std::move(layout));
    }

    save_layout(editable_circuit_->layout());

    update();
    if (editable_circuit_->layout().element_count() < 30) {
        print(editable_circuit_);
    }

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

auto RendererWidget::load_circuit(int id) -> void {
    reset_circuit();
    auto timer = Timer {"", Timer::Unit::ms, 1};

    if (!editable_circuit_) {
        return;
    }
    auto& editable_circuit = editable_circuit_.value();

#ifdef NDEBUG
    constexpr auto debug_build = false;
#else
    constexpr auto debug_build = true;
#endif
    constexpr auto debug_max = 50;
    constexpr auto release_max = 1600;  //  1600;

    if (id == 1) {
        editable_circuit.add_example();
        /*
        editable_circuit.add_standard_logic_item(
            ElementType::or_element, 2, point_t {5, 3}, InsertionMode::insert_or_discard);
        editable_circuit.add_standard_logic_item(ElementType::or_element, 2,
                                                 point_t {15, 6},
                                                 InsertionMode::insert_or_discard);

        editable_circuit.add_line_segments(
            point_t {grid_t {10}, grid_t {10}}, point_t {grid_t {15}, grid_t {12}},
            LineSegmentType::horizontal_first, InsertionMode::insert_or_discard);
        editable_circuit.add_line_segments(
            point_t {grid_t {10}, grid_t {15}}, point_t {grid_t {15}, grid_t {15}},
            LineSegmentType::vertical_first, InsertionMode::insert_or_discard);

        editable_circuit.add_standard_logic_item(ElementType::or_element, 9,
                                                 point_t {20, 4},
                                                 InsertionMode::insert_or_discard);
        */
    }

    if (id == 2) {
        constexpr auto max_value = debug_build ? debug_max : release_max;
        const auto definition = LogicItemDefinition {
            .element_type = ElementType::or_element,
            .input_count = 3,
            .output_count = 1,
            .orientation = orientation_t::right,
        };

        for (auto x : range(5, max_value, 5)) {
            for (auto y : range(5, max_value, 5)) {
                editable_circuit.add_logic_item(definition,
                                                point_t {grid_t {x}, grid_t {y}},
                                                InsertionMode::insert_or_discard);

                editable_circuit.add_line_segments(
                    point_t {grid_t {x + 2}, grid_t {y + 1}},
                    point_t {grid_t {x + 4}, grid_t {y - 1}},
                    LineSegmentType::horizontal_first, InsertionMode::insert_or_discard);

                editable_circuit.add_line_segments(
                    point_t {grid_t {x + 3}, grid_t {y + 1}},
                    point_t {grid_t {x + 5}, grid_t {y + 2}},
                    LineSegmentType::vertical_first, InsertionMode::insert_or_discard);
            }
        }
    }
    if (id == 3) {
        constexpr auto max_value = debug_build ? debug_max : release_max;
        const auto definition = LogicItemDefinition {
            .element_type = ElementType::or_element,
            .input_count = 3,
            .output_count = 1,
            .orientation = orientation_t::right,
        };

        for (auto x : range(5, max_value, 5)) {
            for (auto y : range(5, max_value, 5)) {
                editable_circuit.add_logic_item(definition,
                                                point_t {grid_t {x}, grid_t {y}},
                                                InsertionMode::insert_or_discard);
            }
        }
    }
    if (id == 4) {
        constexpr auto max_value = debug_build ? debug_max : release_max;

        for (auto x : range(5, max_value, 5)) {
            for (auto y : range(5, max_value, 5)) {
                editable_circuit.add_line_segments(
                    point_t {grid_t {x + 2}, grid_t {y + 1}},
                    point_t {grid_t {x + 4}, grid_t {y - 1}},
                    LineSegmentType::horizontal_first, InsertionMode::insert_or_discard);

                editable_circuit.add_line_segments(
                    point_t {grid_t {x + 3}, grid_t {y + 1}},
                    point_t {grid_t {x + 5}, grid_t {y + 2}},
                    LineSegmentType::vertical_first, InsertionMode::insert_or_discard);
            }
        }
    }

    // count & print
    {
        const auto timer_str = timer.format();
        const auto& layout = editable_circuit.layout();

        auto element_count = std::size_t {0};
        auto segment_count = std::size_t {0};

        for (auto element : layout.elements()) {
            if (element.is_wire()) {
                const auto& tree = layout.segment_tree(element.element_id());
                segment_count += tree.segment_count();
            }

            else if (element.is_logic_item()) {
                ++element_count;
            }
        }

        if (layout.element_count() < 10) {
            print(editable_circuit);
        }
        fmt::print("Added {} elements and {} wire segments in {}.\n", element_count,
                   segment_count, timer_str);
    }

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

Q_SLOT void RendererWidget::on_benchmark_timeout() {
    this->update();
}

Q_SLOT void RendererWidget::on_simulation_timeout() {
    if (!editable_circuit_) {
        return;
    }
    if (!simulation_) {
        auto t = Timer {"Generate simulation", Timer::Unit::ms, 3};
        simulation_.emplace(editable_circuit_->layout(), time_rate_,
                            wire_delay_per_distance_);
    }

    const auto timeout
        = timeout_t {std::chrono::milliseconds(simulation_timer_interval_ms_)};
    simulation_->run(timeout);

    // TODO how do we know simulation end is reached?
    // if (event_count > 0) {
    this->update();
    //}
}

auto RendererWidget::pixel_size() const -> QSize {
    double ratio = devicePixelRatioF();
    return QSize(width() * ratio, height() * ratio);
}

void RendererWidget::init() {
    auto window_size = pixel_size();

    qt_image = QImage(window_size.width(), window_size.height(),
                      QImage::Format_ARGB32_Premultiplied);
    qt_image.setDevicePixelRatio(devicePixelRatioF());
    bl_image.createFromData(qt_image.width(), qt_image.height(), BL_FORMAT_PRGB32,
                            qt_image.bits(), qt_image.bytesPerLine());
    bl_info.threadCount = n_threads_;
    render_settings_.view_config.set_device_pixel_ratio(devicePixelRatioF());

    fps_counter_.reset();
}

void RendererWidget::resizeEvent(QResizeEvent* event) {
    if (event == nullptr) {
        return;
    }

    if (event->oldSize() == event->size()) {
        event->accept();
        return;
    }
    init();
}

void RendererWidget::paintEvent([[maybe_unused]] QPaintEvent* event) {
    if (event == nullptr) {
        return;
    }

    if (!this->isVisible()) {
        return;
    }
    if (last_pixel_ratio_ != devicePixelRatioF()) {
        last_pixel_ratio_ = devicePixelRatioF();
        init();
    }

    bl_ctx.begin(bl_image, bl_info);
    const auto& editable_circuit = editable_circuit_.value();

    render_background(bl_ctx, render_settings_);

    if (do_render_circuit_ && simulation_) {
        render_circuit(bl_ctx, render_args_t {
                                   .layout = editable_circuit.layout(),
                                   .schematic = &simulation_->schematic(),
                                   .simulation = &simulation_->simulation(),
                                   .settings = render_settings_,
                               });
    }

    if (do_render_circuit_ && !simulation_) {
        const auto& selection = editable_circuit.selection_builder().selection();
        const auto mask = editable_circuit.selection_builder().create_selection_mask();

        render_circuit(bl_ctx, render_args_t {
                                   .layout = editable_circuit.layout(),
                                   .selection_mask = mask,
                                   .selection = selection,
                                   .settings = render_settings_,
                               });
    }

    if (do_render_collision_cache_) {
        render_editable_circuit_collision_cache(bl_ctx, editable_circuit,
                                                render_settings_);
    }
    if (do_render_connection_cache_) {
        render_editable_circuit_connection_cache(bl_ctx, editable_circuit,
                                                 render_settings_);
    }
    if (do_render_selection_cache_) {
        render_editable_circuit_selection_cache(bl_ctx, editable_circuit,
                                                render_settings_);
    }

    bl_ctx.end();

    QPainter painter(this);
    painter.drawImage(QPoint(0, 0), qt_image);

    fps_counter_.count_event();
}

auto RendererWidget::delete_selected_items() -> void {
    mouse_logic_.reset();

    auto& editable_circuit = editable_circuit_.value();

    auto copy_handle = editable_circuit.create_selection(
        editable_circuit.selection_builder().selection());
    editable_circuit.selection_builder().clear();

    editable_circuit_.value().delete_all(std::move(copy_handle));
    update();
}

auto RendererWidget::select_all_items() -> void {
    if (interaction_state_ == InteractionState::simulation
        || interaction_state_ == InteractionState::not_interactive) {
        return;
    }
    auto& selection_builder = editable_circuit_.value().selection_builder();

    const auto rect = rect_fine_t {point_fine_t {grid_t::min(), grid_t::min()},
                                   point_fine_t {grid_t::max(), grid_t::max()}};

    selection_builder.clear();
    selection_builder.add(SelectionFunction::add, rect);

    update();
}

auto RendererWidget::get_mouse_position() -> point_t {
    if (const auto position
        = to_grid(this->mapFromGlobal(QCursor::pos()), render_settings_.view_config)) {
        return position.value();
    }

    const auto w = this->width();
    const auto h = this->height();

    if (const auto position
        = to_grid(QPoint(w / 2, h / 2), render_settings_.view_config)) {
        return position.value();
    }
    if (const auto position = to_grid(QPoint(0, 0), render_settings_.view_config)) {
        return position.value();
    }
    if (const auto position = to_grid(QPoint(w, h), render_settings_.view_config)) {
        return position.value();
    }

    return point_t {0, 0};
}

auto RendererWidget::copy_selected_items() -> void {
    const auto t = Timer {"", Timer::Unit::ms, 3};

    const auto& layout = editable_circuit_.value().layout();
    const auto& selection = editable_circuit_.value().selection_builder().selection();

    if (!selection.empty()) {
        const auto position = get_mouse_position();
        const auto value = base64_encode(serialize_selected(layout, selection, position));
        QApplication::clipboard()->setText(QString::fromStdString(value));
    }

    print("Copied", selection.selected_logic_items().size(), "logic items and",
          selection.selected_segments().size(), "segments in", t);
}

auto RendererWidget::paste_clipboard_items() -> void {
    if (interaction_state_ == InteractionState::simulation
        || interaction_state_ == InteractionState::not_interactive) {
        return;
    }
    const auto t = Timer {"", Timer::Unit::ms, 3};
    auto& editable_circuit = editable_circuit_.value();

    const auto text = QApplication::clipboard()->text().toStdString();
    const auto binary = base64_decode(text);
    if (binary.empty()) {
        return;
    }

    set_interaction_state(InteractionState::selection);
    reset_interaction_state();

    const auto position = get_mouse_position();
    auto handle
        = add_layout(binary, editable_circuit, InsertionMode::temporary, position);
    if (!handle) {
        return;
    }
    auto split_points = editable_circuit.add_temporary_crosspoints(*handle);
    editable_circuit.change_insertion_mode(editable_circuit.create_selection(*handle),
                                           InsertionMode::collisions);

    editable_circuit.selection_builder().set_selection(*handle.get());

    if (anything_colliding(*handle.get(), editable_circuit.layout())) {
        mouse_logic_.emplace(MouseMoveSelectionLogic::Args {
            .builder = editable_circuit.selection_builder(),
            .editable_circuit = editable_circuit,
            .has_colliding = true,
            .delete_on_cancel = true,
            .split_points = std::move(split_points),
        });
    } else {
        editable_circuit.change_insertion_mode(std::move(handle),
                                               InsertionMode::insert_or_discard);
    }

    const auto& selection = editable_circuit.selection_builder().selection();
    print("Pasted", selection.selected_logic_items().size(), "logic items and",
          selection.selected_segments().size(), "segments in", t);
}

auto RendererWidget::set_new_mouse_logic(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }
    if (event->button() == Qt::LeftButton) {
        if (is_inserting_state(interaction_state_)) {
            if (interaction_state_ == InteractionState::insert_wire) {
                mouse_logic_.emplace(MouseLineInsertLogic::Args {
                    .editable_circuit = editable_circuit_.value(),
                });
                return;
            }
            mouse_logic_.emplace(MouseElementInsertLogic::Args {
                .editable_circuit = editable_circuit_.value(),
                .element_definition
                = to_logic_item_definition(interaction_state_, default_input_count_),
            });
            return;
        }

        if (interaction_state_ == InteractionState::selection) {
            auto& selection_builder = editable_circuit_.value().selection_builder();
            const auto point
                = to_grid_fine(event->position(), render_settings_.view_config);

            if (editable_circuit_->caches().spatial_cache().has_element(point)) {
                if (event->modifiers() == Qt::NoModifier) {
                    mouse_logic_.emplace(MouseMoveSelectionLogic::Args {
                        .builder = selection_builder,
                        .editable_circuit = editable_circuit_.value(),
                    });
                    return;
                }

                mouse_logic_.emplace(MouseSingleSelectionLogic::Args {
                    .builder = selection_builder,
                    .editable_circuit = editable_circuit_.value(),
                });
                return;
            }

            mouse_logic_.emplace(MouseAreaSelectionLogic::Args {
                .parent = this,
                .builder = selection_builder,
                .view_config = render_settings_.view_config,
            });
            return;
        }

        if (interaction_state_ == InteractionState::simulation && simulation_) {
            mouse_logic_.emplace(SimulationInteractionLogic::Args {
                .simulation = simulation_.value(),
            });
        }
    }
}

auto RendererWidget::mousePressEvent(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        mouse_drag_logic_.mouse_press(event->position());
        update();
    }

    else if (event->button() == Qt::LeftButton) {
        if (!mouse_logic_) {
            set_new_mouse_logic(event);
        }
        if (mouse_logic_) {
            const auto grid_position
                = to_grid(event->position(), render_settings_.view_config);
            const auto grid_fine_position
                = to_grid_fine(event->position(), render_settings_.view_config);
            auto double_click = event->type() == QEvent::MouseButtonDblClick;

            std::visit(
                overload {
                    [&](MouseElementInsertLogic& arg) { arg.mouse_press(grid_position); },
                    [&](MouseLineInsertLogic& arg) { arg.mouse_press(grid_position); },
                    [&](MouseAreaSelectionLogic& arg) {
                        arg.mouse_press(event->position(), event->modifiers());
                    },
                    [&](MouseSingleSelectionLogic& arg) {
                        arg.mouse_press(grid_fine_position, double_click);
                    },
                    [&](MouseMoveSelectionLogic& arg) {
                        arg.mouse_press(grid_fine_position, double_click);
                    },
                    [&](SimulationInteractionLogic& arg) {
                        arg.mouse_press(grid_position);
                    },
                },
                *mouse_logic_);
            update();
        }
    }

    else if (event->button() == Qt::RightButton) {
        if (mouse_logic_) {
            mouse_logic_.reset();
        } else {
            editable_circuit_.value().selection_builder().clear();
            if (is_inserting_state(interaction_state_)) {
                set_interaction_state(InteractionState::selection);
            }
        }
        update();
    }

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

auto RendererWidget::mouseMoveEvent(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }

    if (event->buttons() & Qt::MiddleButton) {
        mouse_drag_logic_.mouse_move(event->position());
        update();
    }

    if (mouse_logic_) {
        const auto grid_position
            = to_grid(event->position(), render_settings_.view_config);
        const auto grid_fine_position
            = to_grid_fine(event->position(), render_settings_.view_config);

        std::visit(
            overload {
                [&](MouseElementInsertLogic& arg) { arg.mouse_move(grid_position); },
                [&](MouseLineInsertLogic& arg) { arg.mouse_move(grid_position); },
                [&](MouseAreaSelectionLogic& arg) { arg.mouse_move(event->position()); },
                [&](MouseSingleSelectionLogic& arg) {
                    arg.mouse_move(grid_fine_position);
                },
                [&](MouseMoveSelectionLogic& arg) { arg.mouse_move(grid_fine_position); },
                [&](SimulationInteractionLogic& arg [[maybe_unused]]) {},
            },
            *mouse_logic_);

        update();
    }

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

auto RendererWidget::mouseReleaseEvent(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        mouse_drag_logic_.mouse_release(event->position());
        update();
    }

    else if (event->button() == Qt::LeftButton && mouse_logic_) {
        const auto grid_position
            = to_grid(event->position(), render_settings_.view_config);
        const auto grid_fine_position
            = to_grid_fine(event->position(), render_settings_.view_config);

        bool finished = std::visit(
            overload {
                [&](MouseElementInsertLogic& arg) {
                    arg.mouse_release(grid_position);
                    return true;
                },
                [&](MouseLineInsertLogic& arg) {
                    arg.mouse_release(grid_position);
                    return true;
                },
                [&](MouseAreaSelectionLogic& arg) {
                    arg.mouse_release(event->position());
                    return true;
                },
                [&](MouseSingleSelectionLogic& arg) {
                    arg.mouse_release(grid_fine_position);
                    return true;
                },
                [&](MouseMoveSelectionLogic& arg) {
                    arg.mouse_release(grid_fine_position);
                    return arg.finished();
                },
                [&](SimulationInteractionLogic& arg [[maybe_unused]]) { return true; },
            },
            *mouse_logic_);

        if (finished) {
            mouse_logic_.reset();
        }
        update();
    }

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

auto RendererWidget::wheelEvent(QWheelEvent* event) -> void {
    if (event == nullptr) {
        return;
    }

    auto& view_config = render_settings_.view_config;

    const auto standard_zoom_factor = 1.1;  // zoom factor for one scroll
    const auto standard_scroll_pixel = 45;  // device pixels to scroll for one scroll
    const auto standard_delta = 120.0;      // degree delta for one scroll

    const auto standard_scroll_grid = standard_scroll_pixel / view_config.device_scale();

    // zoom
    if (event->modifiers() == Qt::ControlModifier) {
        const auto delta = event->angleDelta().y() / standard_delta;
        const auto factor = std::exp(delta * std::log(standard_zoom_factor));

        const auto old_grid_point = to_grid_fine(event->position(), view_config);
        view_config.set_device_scale(view_config.device_scale() * factor);
        const auto new_grid_point = to_grid_fine(event->position(), view_config);

        view_config.set_offset(view_config.offset() + new_grid_point - old_grid_point);
        update();
    }

    // standard scroll
    else if (event->modifiers() == Qt::NoModifier) {
        if (event->hasPixelDelta()) {
            // TODO test this
            const auto scale = view_config.device_scale();

            view_config.set_offset(point_fine_t {
                view_config.offset().x + event->pixelDelta().x() / scale,
                view_config.offset().y + event->pixelDelta().y() / scale,
            });
        } else {
            view_config.set_offset(point_fine_t {
                view_config.offset().x
                    + standard_scroll_grid * event->angleDelta().x() / standard_delta,
                view_config.offset().y
                    + standard_scroll_grid * event->angleDelta().y() / standard_delta,
            });
        }
        update();
    }

    // sideway scroll
    else if (event->modifiers() == Qt::ShiftModifier) {
        view_config.set_offset(point_fine_t {
            view_config.offset().x
                + standard_scroll_grid * event->angleDelta().y() / standard_delta,
            view_config.offset().y
                + standard_scroll_grid * event->angleDelta().x() / standard_delta,
        });
        update();
    }

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

auto RendererWidget::keyPressEvent(QKeyEvent* event) -> void {
    if (event->isAutoRepeat()) {
        QWidget::keyPressEvent(event);
        return;
    }

    // Delete
    if (event->key() == Qt::Key_Delete) {
        delete_selected_items();
        event->accept();
    }

    // Escape
    else if (event->key() == Qt::Key_Escape) {
        if (mouse_logic_) {
            mouse_logic_.reset();
        } else {
            editable_circuit_.value().selection_builder().clear();
            if (is_inserting_state(interaction_state_)) {
                set_interaction_state(InteractionState::selection);
            }
        }
        update();
        event->accept();
    }

    // CTRL + A
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_A) {
        select_all_items();
        event->accept();
    }

    // CTRL + C
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_C) {
        copy_selected_items();
        event->accept();
    }

    // CTRL + V
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_V) {
        paste_clipboard_items();
        event->accept();
    }

    // CTRL + X
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_X) {
        copy_selected_items();
        delete_selected_items();
        event->accept();
    }

    // Enter
    else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (mouse_logic_) {
            bool finished = std::visit(
                overload {
                    [&](MouseElementInsertLogic& arg [[maybe_unused]]) { return false; },
                    [&](MouseLineInsertLogic& arg [[maybe_unused]]) { return false; },
                    [&](MouseAreaSelectionLogic& arg [[maybe_unused]]) { return false; },
                    [&](MouseSingleSelectionLogic& arg [[maybe_unused]]) {
                        return false;
                    },
                    [&](MouseMoveSelectionLogic& arg [[maybe_unused]]) {
                        arg.confirm();
                        return arg.finished();
                    },
                    [&](SimulationInteractionLogic& arg [[maybe_unused]]) {
                        return false;
                    },
                },
                *mouse_logic_);

            if (finished) {
                mouse_logic_.reset();
            }

            update();
        }
        event->accept();
    }

    else {
        QWidget::keyPressEvent(event);
    }

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

}  // namespace logicsim
