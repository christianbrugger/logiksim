#include "render_widget.h"

#include "exceptions.h"
#include "layout.h"
#include "range.h"
#include "schematic.h"
#include "simulation.h"

namespace logicsim {

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
    : editable_circuit_ {args.editable_circuit} {}

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
    inserted_key_ = null_element_key;
}

auto MouseElementInsertLogic::remove_last_element() -> void {
    if (inserted_key_ == null_element_key) {
        return;
    }

    editable_circuit_.delete_element(inserted_key_);
    inserted_key_ = null_element_key;
}

auto MouseElementInsertLogic::remove_and_insert(std::optional<point_t> position,
                                                InsertionMode mode) -> void {
    remove_last_element();

    if (position.has_value()) {
        assert(inserted_key_ == null_element_key);

        inserted_key_ = editable_circuit_.add_standard_element(ElementType::or_element, 3,
                                                               *position, mode);
    }
}

//
// Mouse Line Insert Logic
//

MouseLineInsertLogic::MouseLineInsertLogic(Args args) noexcept
    : editable_circuit_ {args.editable_circuit} {}

MouseLineInsertLogic::~MouseLineInsertLogic() {}

auto MouseLineInsertLogic::mouse_press(std::optional<point_t> position) -> void {
    first_position_ = position;
}

auto MouseLineInsertLogic::mouse_move(std::optional<point_t> position) -> void {}

auto MouseLineInsertLogic::mouse_release(std::optional<point_t> position) -> void {
    if (position && first_position_) {
        editable_circuit_.add_line_segment(*first_position_, *position,
                                           LineSegmentType::horizontal_first,
                                           InsertionMode::insert_or_discard);
    }
}

// auto MouseLineInsertLogic::remove_last_element() -> void {}

// auto MouseLineInsertLogic::remove_and_insert(std::optional<point_t> position,
//                                              InsertionMode mode) -> void {}

//
// Mouse Move Selection Logic
//

MouseMoveSelectionLogic::MouseMoveSelectionLogic(Args args)
    : builder_ {args.builder}, editable_circuit_ {args.editable_circuit} {}

MouseMoveSelectionLogic::~MouseMoveSelectionLogic() {
    if (state_ != State::finished) {
        convert_to(InsertionMode::temporary);
        restore_original_positions();
    }
    convert_to(InsertionMode::insert_or_discard);
    builder_.remove_invalid_element_keys();
}

auto MouseMoveSelectionLogic::mouse_press(point_fine_t point) -> void {
    if (state_ == State::waiting_for_first_click) {
        // select element under mouse
        const auto element_under_cursor = editable_circuit_.query_selection(point);
        if (!element_under_cursor.has_value()) {
            builder_.clear();
            return;
        }
        const auto element_key
            = editable_circuit_.to_element_key(element_under_cursor.value());
        const auto is_selected = builder_.selection().is_selected(element_key);

        if (!is_selected) {
            builder_.clear();
            builder_.add(SelectionFunction::add, rect_fine_t {point, point});
        }
    }

    if (state_ == State::waiting_for_first_click
        || state_ == State::waiting_for_confirmation) {
        state_ = State::move_selection;
        last_position_ = point;
    }
}

auto MouseMoveSelectionLogic::mouse_move(point_fine_t point) -> void {
    if (state_ != State::move_selection) {
        return;
    }
    if (!last_position_) {
        return;
    }

    const auto delta_x = round_to<int>(point.x - last_position_->x);
    const auto delta_y = round_to<int>(point.y - last_position_->y);

    if (delta_x == 0 && delta_y == 0) {
        return;
    }

    convert_to(InsertionMode::temporary);

    const auto calculate_new_position = [&, delta_x, delta_y](element_key_t element_key) {
        const auto element_id = editable_circuit_.to_element_id(element_key);
        const auto position = editable_circuit_.layout().position(element_id);

        return std::pair<int, int> {
            position.x.value + delta_x,
            position.y.value + delta_y,
        };
    };

    // check if all positions are valid
    const auto all_valid = std::ranges::all_of(
        get_selection().selected_elements(), [&](element_key_t element_key) {
            const auto [x, y] = calculate_new_position(element_key);
            return editable_circuit_.is_position_valid(element_key, x, y);
        });

    if (all_valid) {
        // move all items
        for (auto&& element_key : get_selection().selected_elements()) {
            const auto [x, y] = calculate_new_position(element_key);
            const auto p = point_t {grid_t {x}, grid_t {y}};
            editable_circuit_.move_or_delete_element(element_key, p);
        }

        last_position_ = point_fine_t {
            last_position_->x + delta_x,
            last_position_->y + delta_y,
        };
    }
}

auto MouseMoveSelectionLogic::mouse_release(point_fine_t point) -> void {
    if (state_ != State::move_selection) {
        return;
    }

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

    state_ = State::finished;
}

auto MouseMoveSelectionLogic::finished() -> bool {
    return state_ == State::finished;
}

auto MouseMoveSelectionLogic::get_selection() -> const Selection& {
    if (!selection_and_positions_baked_) {
        bake_selection_and_positions();
    }
    if (!builder_.is_selection_baked()) [[unlikely]] {
        throw_exception("Selection has been modified after baking.");
    }

    return builder_.selection();
}

auto MouseMoveSelectionLogic::bake_selection_and_positions() -> void {
    if (selection_and_positions_baked_) {
        return;
    }
    selection_and_positions_baked_ = true;

    // bake selection, so we can move the elements
    builder_.bake_selection();
    const auto& selection = get_selection();
    const auto selected_keys = selection.selected_elements();

    // store initial positions
    if (!original_positions_.empty()) [[unlikely]] {
        throw_exception("Original positions need to be empty.");
    }
    original_positions_.reserve(selected_keys.size());
    std::ranges::transform(selected_keys, std::back_inserter(original_positions_),
                           [&](element_key_t element_key) {
                               const auto element_id
                                   = editable_circuit_.to_element_id(element_key);
                               return editable_circuit_.layout().position(element_id);
                           });
}

auto MouseMoveSelectionLogic::convert_to(InsertionMode mode) -> void {
    if (insertion_mode_ == mode) {
        return;
    }
    insertion_mode_ = mode;

    for (auto&& element_key : get_selection().selected_elements()) {
        editable_circuit_.change_insertion_mode(element_key, mode);
    }
}

auto MouseMoveSelectionLogic::restore_original_positions() -> void {
    const auto& selection = get_selection();
    const auto selected_keys = selection.selected_elements();

    if (selected_keys.size() != original_positions_.size()) [[unlikely]] {
        throw_exception("Number of original positions doesn't match selection.");
    }

    for (auto i : range(selected_keys.size())) {
        bool moved = editable_circuit_.move_or_delete_element(selected_keys[i],
                                                              original_positions_[i]);
        if (!moved) [[unlikely]] {
            throw_exception("item was not revertable to old positions.");
        }
    }
}

auto MouseMoveSelectionLogic::calculate_any_element_colliding() -> bool {
    const auto element_colliding = [&](element_key_t element_key) {
        const auto element_id = editable_circuit_.to_element_id(element_key);
        return editable_circuit_.layout().display_state(element_id)
               == display_state_t::new_colliding;
    };

    return std::ranges::any_of(get_selection().selected_elements(), element_colliding);
}

//
// Mouse Item Selection Logic
//

MouseSingleSelectionLogic::MouseSingleSelectionLogic(Args args)
    : builder_ {args.builder} {}

auto MouseSingleSelectionLogic::mouse_press(point_fine_t point,
                                            Qt::KeyboardModifiers modifiers) -> void {
    builder_.add(SelectionFunction::toggle, rect_fine_t {point, point});
}

auto MouseSingleSelectionLogic::mouse_move(point_fine_t point) -> void {}

auto MouseSingleSelectionLogic::mouse_release(point_fine_t point) -> void {}

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
// Render Widget
//

auto format(InteractionState state) -> std::string {
    switch (state) {
        using enum InteractionState;

        case not_interactive:
            return "not_interactive";
        case select:
            return "select";
        case element_insert:
            return "element_insert";
        case line_insert:
            return "line_insert";
    }
    throw_exception("Don't know how to convert InteractionState to string.");
}

RendererWidget::RendererWidget(QWidget* parent)
    : QWidget(parent),
      last_pixel_ratio_ {devicePixelRatioF()},
      animation_start_ {animation_clock::now()},
      mouse_drag_logic_ {MouseDragLogic::Args {render_settings_.view_config}} {
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    // accept focus so keyboard signals gets fired
    setFocusPolicy(Qt::StrongFocus);

    connect(&timer_, &QTimer::timeout, this, &RendererWidget::on_timeout);

    render_settings_.view_config.set_device_scale(18);
    reset_circuit();
}

auto RendererWidget::set_do_benchmark(bool value) -> void {
    do_benchmark_ = value;

    if (value) {
        timer_.start();
    } else {
        timer_.stop();
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
    if (interaction_state_ == state) {
        return;
    }
    interaction_state_ = state;

    mouse_logic_.reset();
    editable_circuit_.value().selection_builder().clear();
    update();
}

auto RendererWidget::fps() const -> double {
    return fps_counter_.events_per_second();
}

auto RendererWidget::pixel_scale() const -> double {
    return render_settings_.view_config.pixel_scale();
}

auto RendererWidget::reset_circuit() -> void {
    circuit_index_ = CircuitIndex {};
    editable_circuit_.emplace(circuit_index_.borrow_schematic(circuit_id_),
                              circuit_index_.borrow_layout(circuit_id_));

    {
        auto& editable_circuit = editable_circuit_.value();

        // auto tree1 = LineTree({point_t {7, 3}, point_t {10, 3}, point_t {10, 1}});
        // auto tree2 = LineTree({point_t {10, 3}, point_t {10, 7}, point_t {4, 7},
        //                        point_t {4, 4}, point_t {5, 4}});
        //// auto tree1 = LineTree({point_t {10, 10}, point_t {10, 12}, point_t {8,
        //// 12}}); auto tree2 = LineTree({point_t {10, 12}, point_t {12, 12},
        //// point_t {12, 14}});
        // auto line_tree = merge({tree1, tree2}).value_or(LineTree {});

        editable_circuit.add_standard_element(ElementType::or_element, 2, point_t {5, 3},
                                              InsertionMode::insert_or_discard);
        const auto element1 = editable_circuit.add_standard_element(
            ElementType::or_element, 2, point_t {15, 6},
            InsertionMode::insert_or_discard);
        // editable_circuit.add_wire(std::move(line_tree));
        // editable_circuit.add_wire(
        //     LineTree({point_t {8, 1}, point_t {8, 2}, point_t {15, 2}, point_t {15,
        //     4}}));
        //  editable_circuit.add_wire(LineTree({point_t {15, 2}, point_t {8, 2}}));
        editable_circuit.delete_element(element1);

        const auto added = editable_circuit.add_standard_element(
            ElementType::or_element, 9, point_t {20, 4},
            InsertionMode::insert_or_discard);
        fmt::print("added = {}\n", added);

        // editable_circuit.add_wire(LineTree({point_t {5, 20}, point_t {20, 20}}));
        // editable_circuit.add_wire(LineTree({point_t {20, 30}, point_t {5, 30}}));

        fmt::print("{}\n", editable_circuit);
        editable_circuit.schematic().validate(Schematic::validate_all);

        {
            auto timer = Timer {};
            auto count = 0;
#ifdef NDEBUG
            constexpr auto max_value = 2000;
#else
            constexpr auto max_value = 200;
#endif
            for (auto x : range(100, max_value, 5)) {
                for (auto y : range(100, max_value, 5)) {
                    editable_circuit.add_standard_element(
                        ElementType::or_element, 3, point_t {grid_t {x}, grid_t {y}},
                        InsertionMode::insert_or_discard);

                    editable_circuit.add_line_segment(
                        point_t {grid_t {x + 2}, grid_t {y + 1}},
                        point_t {grid_t {x + 4}, grid_t {y + 0}},
                        LineSegmentType::horizontal_first,
                        InsertionMode::insert_or_discard);

                    count++;
                }
            }
            fmt::print("Added {} elements in {}.\n", count, timer.format());
        }
    }
}

Q_SLOT void RendererWidget::on_timeout() {
    this->update();
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
};

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

    // build circuit
    auto& editable_circuit = editable_circuit_.value();

    // old behaviour

    /*
    const double animation_seconds
        = std::chrono::duration<double>(animation_clock::now() - animation_start_)
              .count();
    const double animation_frame = fmod(animation_seconds / 5.0, 1.0);

    Schematic schematic;

    const auto elem0 = schematic.add_element(ElementType::or_element, 2, 1);
    const auto line0 = schematic.add_element(ElementType::wire, 1, 2);
    elem0.output(connection_id_t {0}).connect(line0.input(connection_id_t {0}));
    line0.output(connection_id_t {0}).connect(elem0.input(connection_id_t {1}));

    add_output_placeholders(schematic);
    auto simulation = Simulation {schematic};
    simulation.print_events = true;

    simulation.set_output_delay(elem0.output(connection_id_t {0}), delay_t
    {10us}); simulation.set_output_delay(line0.output(connection_id_t {0}),
    delay_t {40us}); simulation.set_output_delay(line0.output(connection_id_t
    {0}), delay_t {60us}); simulation.set_history_length(line0, delay_t {60us});

    simulation.initialize();
    simulation.submit_event(elem0.input(connection_id_t {0}), 100us, true);
    simulation.submit_event(elem0.input(connection_id_t {0}), 105us, false);
    simulation.submit_event(elem0.input(connection_id_t {0}), 110us, true);
    simulation.submit_event(elem0.input(connection_id_t {0}), 500us, false);

    // TODO use gsl narrow
    auto end_time_ns = static_cast<uint64_t>(90'000 + animation_frame * 120'000);
    const auto end_time [[maybe_unused]] = end_time_ns * 1ns;
    // simulation.run(end_time);

    simulation.run(125us);
    // simulation.run(130us);
    // simulation.run(600us);
    timer_.stop();

    // create layout
    auto layout = Layout {};
    for (auto _ [[maybe_unused]] : range(schematic.element_count())) {
        layout.add_default_element();
    }
    layout.set_position(elem0, point_t {5, 3});

    auto tree1 = LineTree({point_t {10, 10}, point_t {10, 12}, point_t {8, 12}});
    auto tree2 = LineTree({point_t {10, 12}, point_t {12, 12}, point_t {12, 14}});
    auto line_tree = merge({tree1, tree2}).value_or(LineTree {});
    layout.set_line_tree(line0, std::move(line_tree));

    // int w = qt_image.width();
    // int h = qt_image.height();
    */
    bl_ctx.begin(bl_image, bl_info);

    render_background(bl_ctx, render_settings_);

    if (do_render_circuit_) {
        const auto& selection = editable_circuit.selection_builder().selection();
        fmt::print("{}\n", selection);

        const auto mask = editable_circuit.selection_builder().create_selection_mask();

        // auto simulation = Simulation {editable_circuit.schematic()};
        render_circuit(bl_ctx, render_args_t {
                                   .schematic = editable_circuit.schematic(),
                                   .layout = editable_circuit.layout(),
                                   .key_resolver = KeyResolver {editable_circuit},
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

    auto& selection_builder = editable_circuit_.value().selection_builder();

    auto&& selected_keys = selection_builder.selection().selected_elements();
    for (auto&& element_key : selected_keys) {
        editable_circuit_.value().delete_element(element_key);
    }

    selection_builder.clear();
    update();

#ifndef NDEBUG
    editable_circuit_->validate();
#endif
}

auto RendererWidget::select_all_items() -> void {
    if (interaction_state_ != InteractionState::select) {
        return;
    }
    auto& selection_builder = editable_circuit_.value().selection_builder();

    const auto rect = rect_fine_t {point_fine_t {grid_t::min(), grid_t::min()},
                                   point_fine_t {grid_t::max(), grid_t::max()}};

    selection_builder.clear();
    selection_builder.add(SelectionFunction::add, rect);

    update();
}

auto RendererWidget::set_new_mouse_logic(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (interaction_state_ == InteractionState::element_insert) {
        mouse_logic_.emplace(MouseElementInsertLogic::Args {
            .editable_circuit = editable_circuit_.value(),
        });
        return;
    }

    if (interaction_state_ == InteractionState::line_insert) {
        mouse_logic_.emplace(MouseLineInsertLogic::Args {
            .editable_circuit = editable_circuit_.value(),
        });
        return;
    }

    if (interaction_state_ == InteractionState::select) {
        auto& selection_builder = editable_circuit_.value().selection_builder();
        const auto point = to_grid_fine(event->position(), render_settings_.view_config);
        const bool has_element_under_cursor
            = editable_circuit_.value().query_selection(point).has_value();

        if (has_element_under_cursor) {
            if (event->modifiers() == Qt::NoModifier) {
                mouse_logic_.emplace(MouseMoveSelectionLogic::Args {
                    .builder = selection_builder,
                    .editable_circuit = editable_circuit_.value(),
                });
                return;
            }

            mouse_logic_.emplace(MouseSingleSelectionLogic::Args {
                .builder = selection_builder,
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
}

auto RendererWidget::mousePressEvent(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }
    if (event->button() == Qt::MiddleButton) {
        mouse_drag_logic_.mouse_press(event->position());
        update();
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (!mouse_logic_.has_value()) {
        set_new_mouse_logic(event);
    }
    if (mouse_logic_) {
        const auto grid_position
            = to_grid(event->position(), render_settings_.view_config);
        const auto grid_fine_position
            = to_grid_fine(event->position(), render_settings_.view_config);

        std::visit(
            overload {
                [&](MouseElementInsertLogic& arg) { arg.mouse_press(grid_position); },
                [&](MouseLineInsertLogic& arg) { arg.mouse_press(grid_position); },
                [&](MouseAreaSelectionLogic& arg) {
                    arg.mouse_press(event->position(), event->modifiers());
                },
                [&](MouseSingleSelectionLogic& arg) {
                    arg.mouse_press(grid_fine_position, event->modifiers());
                },
                [&](MouseMoveSelectionLogic& arg) {
                    arg.mouse_press(grid_fine_position);
                },
            },
            *mouse_logic_);
        update();
    }
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
            },
            *mouse_logic_);

        update();
    }
}

auto RendererWidget::mouseReleaseEvent(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }
    if (event->button() == Qt::MiddleButton) {
        mouse_drag_logic_.mouse_release(event->position());
        update();
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (mouse_logic_) {
        const auto grid_position
            = to_grid(event->position(), render_settings_.view_config);
        const auto grid_fine_position
            = to_grid_fine(event->position(), render_settings_.view_config);

        bool finished = std::visit(overload {
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
                                   },
                                   *mouse_logic_);

        if (finished) {
            mouse_logic_.reset();
#ifndef NDEBUG
            editable_circuit_->validate();
#endif
        }
        update();
    }
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
}

auto RendererWidget::keyPressEvent(QKeyEvent* event) -> void {
    // Delete
    if (event->key() == Qt::Key_Delete) {
        delete_selected_items();
        return;
    }

    // Escape
    if (event->key() == Qt::Key_Escape) {
        if (mouse_logic_) {
            mouse_logic_.reset();
#ifndef NDEBUG
            editable_circuit_->validate();
#endif
        } else {
            editable_circuit_.value().selection_builder().clear();
        }
        update();
        return;
    }

    // CTRL + A
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_A) {
        select_all_items();
        return;
    }

    // Enter
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (mouse_logic_) {
            bool finished
                = std::visit(overload {
                                 [&](MouseElementInsertLogic& arg) { return false; },
                                 [&](MouseLineInsertLogic& arg) { return false; },
                                 [&](MouseAreaSelectionLogic& arg) { return false; },
                                 [&](MouseSingleSelectionLogic& arg) { return false; },
                                 [&](MouseMoveSelectionLogic& arg) {
                                     arg.confirm();
                                     return arg.finished();
                                 },
                             },
                             *mouse_logic_);

            if (finished) {
                mouse_logic_.reset();
#ifndef NDEBUG
                editable_circuit_->validate();
#endif
            }

            update();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

}  // namespace logicsim
