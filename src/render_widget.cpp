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
        config_.offset
            += to_grid_fine(position, config_) - to_grid_fine(*last_position, config_);
    }
    last_position = position;
}

auto MouseDragLogic::mouse_release(QPointF position) -> void {
    mouse_move(position);
    last_position = std::nullopt;
}

//
// Mouse Insert Logic
//

MouseInsertLogic::MouseInsertLogic(Args args) noexcept
    : editable_circuit_ {args.editable_circuit} {}

MouseInsertLogic::~MouseInsertLogic() {
    remove_last_element();
}

auto MouseInsertLogic::mouse_press(std::optional<point_t> position) -> void {
    remove_and_insert(position, InsertionMode::collisions);
}

auto MouseInsertLogic::mouse_move(std::optional<point_t> position) -> void {
    remove_and_insert(position, InsertionMode::collisions);
}

auto MouseInsertLogic::mouse_release(std::optional<point_t> position) -> void {
    remove_and_insert(position, InsertionMode::insert_or_discard);
    inserted_key_ = null_element_key;
}

auto MouseInsertLogic::remove_last_element() -> void {
    if (inserted_key_ != null_element_key) {
        const auto element_id = editable_circuit_.to_element_id(inserted_key_);
        editable_circuit_.swap_and_delete_element(element_id);
        inserted_key_ = null_element_key;
    }
}

auto MouseInsertLogic::remove_and_insert(std::optional<point_t> position,
                                         InsertionMode mode) -> void {
    remove_last_element();

    if (position.has_value()) {
        assert(inserted_key_ == null_element_key);

        inserted_key_ = editable_circuit_.add_standard_element(ElementType::or_element, 3,
                                                               *position, mode);
    }
}

//
// Selection Manager
//

auto SelectionManager::clear() -> void {
    initial_selected_.clear();
    operations_.clear();
}

auto SelectionManager::add(SelectionFunction function, rect_fine_t rect) -> void {
    operations_.emplace_back(operation_t {function, rect});
}

auto SelectionManager::update_last(rect_fine_t rect) -> void {
    if (operations_.empty()) [[unlikely]] {
        throw_exception("Cannot update with empty operations.");
    }
    operations_.back().rect = rect;
}

namespace {
auto apply_function(SelectionManager::selection_mask_t& selection,
                    const EditableCircuit& editable_circuit,
                    SelectionManager::operation_t operation) -> void {
    auto elements = editable_circuit.query_selection(operation.rect);
    std::ranges::sort(elements);

    if (elements.size() == 0) {
        return;
    }

    // bound checking
    if (elements.front().value < 0 || elements.back().value >= std::ssize(selection))
        [[unlikely]] {
        throw_exception("Element ids are out of selection bounds.");
    }

    if (operation.function == SelectionFunction::toggle) {
        for (auto&& element_id : elements) {
            selection[element_id.value] ^= true;
        }
    }

    if (operation.function == SelectionFunction::add) {
        for (auto&& element_id : elements) {
            selection[element_id.value] = true;
        }
    }

    if (operation.function == SelectionFunction::substract) {
        for (auto&& element_id : elements) {
            selection[element_id.value] = false;
        }
    }
}
}  // namespace

auto SelectionManager::claculate_item_selected(
    element_id_t element_id, const EditableCircuit& editable_circuit) const -> bool {
    if (element_id < element_id_t {0}) [[unlikely]] {
        throw_exception("Invalid element id");
    }

    const auto selections = create_selection_mask(editable_circuit);

    if (element_id.value >= std::ssize(selections)) {
        return false;
    }

    return selections.at(element_id.value);
}

auto SelectionManager::create_selection_mask(
    const EditableCircuit& editable_circuit) const -> selection_mask_t {
    if (operations_.empty()) {
        return {};
    }

    const auto element_count = editable_circuit.schematic().element_count();
    auto selection = selection_mask_t(element_count, false);

    for (auto&& operation : operations_) {
        apply_function(selection, editable_circuit, operation);
    }
    return selection;
}

auto SelectionManager::bake_selection(const EditableCircuit& editable_circuit) -> void {
    using std::swap;

    auto selected_keys = calculate_selected_keys(editable_circuit);

    swap(initial_selected_, selected_keys);
}

auto SelectionManager::calculate_selected_ids(
    const EditableCircuit& editable_circuit) const -> std::vector<element_id_t> {
    const auto selection = create_selection_mask(editable_circuit);
    const auto maximum_id = gsl::narrow<element_id_t::value_type>(selection.size());

    // TODO create algorithm
    auto selected_ids = std::vector<element_id_t> {};
    for (auto i : range(maximum_id)) {
        if (selection[i]) {
            selected_ids.push_back(element_id_t {i});
        }
    };

    return selected_ids;
}

auto SelectionManager::calculate_selected_keys(
    const EditableCircuit& editable_circuit) const -> std::vector<element_key_t> {
    const auto selected_ids = calculate_selected_ids(editable_circuit);
    return editable_circuit.to_element_keys(selected_ids);
}

//
// Mouse Move Selection Logic
//

MouseMoveSelectionLogic::MouseMoveSelectionLogic(Args args)
    : manager_ {args.manager}, editable_circuit_ {args.editable_circuit} {}

auto MouseMoveSelectionLogic::mouse_press(point_fine_t point) -> void {
    const auto element_under_cursor = editable_circuit_.query_selection(point);

    if (!element_under_cursor.has_value()) {
        manager_.clear();
        return;
    }

    const auto element_is_selected = manager_.claculate_item_selected(
        element_under_cursor.value(), editable_circuit_);

    if (!element_is_selected) {
        manager_.clear();
        manager_.add(SelectionFunction::add, rect_fine_t {point, point});
    }

    last_position_ = point;
}

auto MouseMoveSelectionLogic::mouse_move(point_fine_t point) -> void {
    if (!last_position_) {
        return;
    }

    const auto delta_x = round_to<int>(point.x - last_position_->x);
    const auto delta_y = round_to<int>(point.y - last_position_->y);

    if (delta_x == 0 && delta_y == 0) {
        return;
    }

    convert_selection();

    fmt::print("{} {}\n", delta_x, delta_y);
}

auto MouseMoveSelectionLogic::mouse_release(point_fine_t point) -> void {}

auto MouseMoveSelectionLogic::convert_selection() -> void {
    if (converted_) {
        return;
    }

    converted_ = true;
}

//
// Mouse Item Selection Logic
//

MouseSingleSelectionLogic::MouseSingleSelectionLogic(Args args)
    : manager_ {args.manager} {}

auto MouseSingleSelectionLogic::mouse_press(point_fine_t point,
                                            Qt::KeyboardModifiers modifiers) -> void {
    manager_.add(SelectionFunction::toggle, rect_fine_t {point, point});
}

auto MouseSingleSelectionLogic::mouse_move(point_fine_t point) -> void {}

auto MouseSingleSelectionLogic::mouse_release(point_fine_t point) -> void {}

//
// Mouse Area Selection Logic
//

MouseAreaSelectionLogic::MouseAreaSelectionLogic(Args args)
    : manager_ {args.manager},
      view_config_ {args.view_config},
      band_ {QRubberBand::Rectangle, args.parent} {}

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
        manager_.clear();
    }

    manager_.add(function, rect_fine_t {p0, p0});
    first_position_ = p0;
}

auto MouseAreaSelectionLogic::mouse_move(QPointF position) -> void {
    update_mouse_position(position);
}

auto MouseAreaSelectionLogic::mouse_release(QPointF position) -> void {
    update_mouse_position(position);
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

    manager_.update_last(grid_rect);
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
        case insert:
            return "insert";
    }
    throw_exception("Don't know how to convert InteractionState to string.");
}

RendererWidget::RendererWidget(QWidget* parent)
    : QWidget(parent),
      last_pixel_ratio_ {devicePixelRatioF()},
      animation_start_ {animation_clock::now()} {
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    connect(&timer_, &QTimer::timeout, this, &RendererWidget::on_timeout);

    render_settings_.view_config.scale = 18;

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

    selection_manager_.clear();
    mouse_logic_.reset();
    update();
}

auto RendererWidget::fps() const -> double {
    return fps_counter_.events_per_second();
}

auto RendererWidget::scale() const -> double {
    return render_settings_.view_config.scale;
}

auto RendererWidget::reset_circuit() -> void {
    circuit_index_ = CircuitIndex {};
    editable_circuit_ = EditableCircuit {circuit_index_.borrow_schematic(circuit_id_),
                                         circuit_index_.borrow_layout(circuit_id_)};

    {
        auto& editable_circuit = editable_circuit_.value();

        auto tree1 = LineTree({point_t {7, 3}, point_t {10, 3}, point_t {10, 1}});
        auto tree2 = LineTree({point_t {10, 3}, point_t {10, 7}, point_t {4, 7},
                               point_t {4, 4}, point_t {5, 4}});
        // auto tree1 = LineTree({point_t {10, 10}, point_t {10, 12}, point_t {8,
        // 12}}); auto tree2 = LineTree({point_t {10, 12}, point_t {12, 12},
        // point_t {12, 14}});
        auto line_tree = merge({tree1, tree2}).value_or(LineTree {});

        editable_circuit.add_standard_element(ElementType::or_element, 2, point_t {5, 3},
                                              InsertionMode::insert_or_discard);
        editable_circuit.add_standard_element(ElementType::or_element, 2, point_t {15, 6},
                                              InsertionMode::insert_or_discard);
        editable_circuit.add_wire(std::move(line_tree));
        editable_circuit.add_wire(
            LineTree({point_t {8, 1}, point_t {8, 2}, point_t {15, 2}, point_t {15, 4}}));
        // editable_circuit.add_wire(LineTree({point_t {15, 2}, point_t {8, 2}}));
        editable_circuit.swap_and_delete_element(element_id_t {2});

        auto added = editable_circuit.add_standard_element(
            ElementType::or_element, 9, point_t {20, 4},
            InsertionMode::insert_or_discard);
        fmt::print("added = {}\n", added);

        editable_circuit.add_wire(LineTree({point_t {5, 20}, point_t {20, 20}}));
        editable_circuit.add_wire(LineTree({point_t {20, 30}, point_t {5, 30}}));

        fmt::print("{}\n", editable_circuit);
        editable_circuit.schematic().validate(Schematic::validate_all);

        {
            auto timer = Timer {};
            auto count = 0;
            for (auto x : range(100, 200, 5)) {
                for (auto y : range(100, 200, 5)) {
                    editable_circuit.add_standard_element(
                        ElementType::or_element, 3, point_t {grid_t {x}, grid_t {y}},
                        InsertionMode::insert_or_discard);
                    count++;
                }
            }
            fmt::print("Added {} elements in {}.", count, timer.format());
        }
    }
}

Q_SLOT void RendererWidget::on_timeout() {
    this->update();
}

QSize RendererWidget::size_pixels() {
    double ratio = devicePixelRatioF();
    return QSize(width() * ratio, height() * ratio);
}

void RendererWidget::init() {
    auto window_size = size_pixels();

    qt_image = QImage(window_size.width(), window_size.height(),
                      QImage::Format_ARGB32_Premultiplied);
    qt_image.setDevicePixelRatio(devicePixelRatioF());
    bl_image.createFromData(qt_image.width(), qt_image.height(), BL_FORMAT_PRGB32,
                            qt_image.bits(), qt_image.bytesPerLine());
    bl_info.threadCount = n_threads_;

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

    if (last_pixel_ratio_ != devicePixelRatioF()) {
        last_pixel_ratio_ = devicePixelRatioF();
        init();
    }
    if (!this->isVisible()) {
        return;
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

    const auto selection_mask
        = selection_manager_.create_selection_mask(editable_circuit);

    bl_ctx.begin(bl_image, bl_info);

    render_background(bl_ctx, render_settings_);

    if (do_render_circuit_) {
        // auto simulation = Simulation {editable_circuit.schematic()};
        render_circuit(bl_ctx, editable_circuit.schematic(), editable_circuit.layout(),
                       nullptr, selection_mask, render_settings_);
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

auto RendererWidget::set_new_mouse_logic(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        mouse_logic_.emplace(MouseDragLogic::Args {
            .view_config = render_settings_.view_config,
        });
        return;
    }

    if (interaction_state_ == InteractionState::insert
        && event->button() == Qt::LeftButton) {
        mouse_logic_.emplace(MouseInsertLogic::Args {
            .editable_circuit = editable_circuit_.value(),
        });
        return;
    }

    if (interaction_state_ == InteractionState::select
        && event->button() == Qt::LeftButton) {
        const auto point = to_grid_fine(event->position(), render_settings_.view_config);
        const bool has_element_under_cursor
            = editable_circuit_.value().query_selection(point).has_value();

        if (has_element_under_cursor) {
            if (event->modifiers() == Qt::NoModifier) {
                mouse_logic_.emplace(MouseMoveSelectionLogic::Args {
                    .manager = selection_manager_,
                    .editable_circuit = editable_circuit_.value(),
                });
                return;
            }

            mouse_logic_.emplace(MouseSingleSelectionLogic::Args {
                .manager = selection_manager_,
            });
            return;
        }

        mouse_logic_.emplace(MouseAreaSelectionLogic::Args {
            .parent = this,
            .manager = selection_manager_,
            .view_config = render_settings_.view_config,
        });
        return;
    }
}

auto RendererWidget::mousePressEvent(QMouseEvent* event) -> void {
    if (event == nullptr) {
        return;
    }

    set_new_mouse_logic(event);

    // visit mouse logic
    if (mouse_logic_) {
        const auto grid_position
            = to_grid(event->position(), render_settings_.view_config);
        const auto grid_fine_position
            = to_grid_fine(event->position(), render_settings_.view_config);

        std::visit(overload {
                       [&](MouseDragLogic& arg) { arg.mouse_press(event->position()); },
                       [&](MouseInsertLogic& arg) { arg.mouse_press(grid_position); },
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

    if (mouse_logic_) {
        const auto grid_position
            = to_grid(event->position(), render_settings_.view_config);
        const auto grid_fine_position
            = to_grid_fine(event->position(), render_settings_.view_config);

        std::visit(
            overload {
                [&](MouseDragLogic& arg) { arg.mouse_move(event->position()); },
                [&](MouseInsertLogic& arg) { arg.mouse_move(grid_position); },
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

    if (mouse_logic_) {
        const auto grid_position
            = to_grid(event->position(), render_settings_.view_config);
        const auto grid_fine_position
            = to_grid_fine(event->position(), render_settings_.view_config);

        std::visit(overload {
                       [&](MouseDragLogic& arg) { arg.mouse_release(event->position()); },
                       [&](MouseInsertLogic& arg) { arg.mouse_release(grid_position); },
                       [&](MouseAreaSelectionLogic& arg) {
                           arg.mouse_release(event->position());
                       },
                       [&](MouseSingleSelectionLogic& arg) {
                           arg.mouse_release(grid_fine_position);
                       },
                       [&](MouseMoveSelectionLogic& arg) {
                           arg.mouse_release(grid_fine_position);
                       },
                   },
                   *mouse_logic_);

        update();
    }

    // delete mouse logic
    mouse_logic_ = std::nullopt;
}

auto RendererWidget::wheelEvent(QWheelEvent* event) -> void {
    if (event == nullptr) {
        return;
    }

    const auto standard_delta = 120.0;      // standard delta for one scroll
    const auto standard_zoom_factor = 1.1;  // zoom factor for one scroll
    const auto standard_scroll_pixel = 20;  // pixels to scroll for one scroll

    const auto standard_scroll_grid
        = standard_scroll_pixel / render_settings_.view_config.scale;

    // zoom
    if (event->modifiers() == Qt::ControlModifier) {
        const auto delta = event->angleDelta().y() / standard_delta;
        const auto factor = std::exp(delta * std::log(standard_zoom_factor));

        const auto old_grid_point
            = to_grid_fine(event->position(), render_settings_.view_config);

        render_settings_.view_config.scale *= factor;

        const auto new_grid_point
            = to_grid_fine(event->position(), render_settings_.view_config);

        render_settings_.view_config.offset += new_grid_point - old_grid_point;
        update();
    }

    // standard scroll
    else if (event->modifiers() == Qt::NoModifier) {
        if (event->hasPixelDelta()) {
            // TODO test this
            render_settings_.view_config.offset.x += event->pixelDelta().x();
            render_settings_.view_config.offset.y += event->pixelDelta().y();
        } else {
            render_settings_.view_config.offset.x
                += standard_scroll_grid * event->angleDelta().x() / standard_delta;
            render_settings_.view_config.offset.y
                += standard_scroll_grid * event->angleDelta().y() / standard_delta;
        }
        update();
    }

    // inverted scroll
    else if (event->modifiers() == Qt::ShiftModifier) {
        render_settings_.view_config.offset.x
            += standard_scroll_grid * event->angleDelta().y() / standard_delta;
        render_settings_.view_config.offset.y
            += standard_scroll_grid * event->angleDelta().x() / standard_delta;
        update();
    }
}

}  // namespace logicsim
