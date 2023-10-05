#include "setting_handle.h"

#include "algorithm/range.h"
#include "algorithm/round.h"
#include "editable_circuit/editable_circuit.h"
#include "editable_circuit/selection.h"
#include "geometry/rect.h"
#include "layout.h"
#include "layout_calculation.h"
#include "resource.h"
#include "scene.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>

namespace logicsim {
auto setting_handle_position(const Layout& layout, element_id_t element_id)
    -> std::optional<setting_handle_t> {
    const auto element = layout.element(element_id);

    switch (layout.element_type(element_id)) {
        using enum ElementType;

        case clock_generator: {
            // constexpr auto overdraw = defaults::logic_item_body_overdraw;
            constexpr auto handle_size = defaults::setting_handle_size;
            // constexpr auto margin = defaults::setting_handle_margin;

            const auto width = grid_fine_t {5.0};
            const auto height = grid_fine_t {4.0};

            return setting_handle_t {
                .position =
                    transform(element.position(), element.orientation(),
                              point_fine_t {
                                  // width - handle_size / 2.0 - margin,
                                  // margin + handle_size / 2.0,
                                  // height + overdraw - handle_size / 2.0 - margin,
                                  width / 2.0,
                                  height / 2.0 + handle_size / 2.0,
                              }),
                .icon = icon_t::setting_handle_clock_generator,
                .element_id = element_id,
            };
        }

        case unused:
        case placeholder:
        case wire:

        case buffer_element:
        case and_element:
        case or_element:
        case xor_element:

        case button:
        case led:
        case display_number:
        case display_ascii:

        case flipflop_jk:
        case shift_register:
        case latch_d:
        case flipflop_d:
        case flipflop_ms_d:

        case sub_circuit:
            return {};
    };

    throw_exception("unknown ElementType in setting_handle_position");
}

namespace {

auto get_single_logic_item(const Selection& selection) -> element_id_t {
    if (selection.selected_logic_items().size() != 1 ||
        !selection.selected_segments().empty()) {
        return null_element;
    }
    return selection.selected_logic_items().front();
}

}  // namespace

auto setting_handle_position(const Layout& layout, const Selection& selection)
    -> std::optional<setting_handle_t> {
    const auto element_id = get_single_logic_item(selection);
    if (!element_id) {
        return {};
    }
    if (layout.display_state(element_id) != display_state_t::normal) {
        return {};
    }

    return setting_handle_position(layout, element_id);
}

auto setting_handle_rect(setting_handle_t handle) -> rect_fine_t {
    return to_rect(handle.position, defaults::setting_handle_size);
}

auto is_colliding(setting_handle_t handle, point_fine_t position) -> bool {
    return is_colliding(position, setting_handle_rect(handle));
}

auto get_colliding_setting_handle(point_fine_t position, const Layout& layout,
                                  const Selection& selection)
    -> std::optional<setting_handle_t> {
    const auto handle = setting_handle_position(layout, selection);

    if (handle && is_colliding(*handle, position)) {
        return handle;
    }

    return {};
}

//
// Setting Widget Registrar
//

SettingWidgetRegistry::SettingWidgetRegistry(QWidget* parent,
                                             EditableCircuit& editable_circuit)
    : parent_ {parent}, editable_circuit_ {editable_circuit} {
    connect(&cleanup_timer_, &QTimer::timeout, this,
            &SettingWidgetRegistry::on_cleanup_timeout);

    cleanup_timer_.setInterval(250);
    cleanup_timer_.start();
}

SettingWidgetRegistry::~SettingWidgetRegistry() {
    close_all();
}

auto SettingWidgetRegistry::show_setting_dialog(setting_handle_t setting_handle) -> void {
    // activate existing
    for (auto&& [widget, selection_handle] : map_) {
        if (selection_handle->is_selected(setting_handle.element_id)) {
            widget->show();
            widget->activateWindow();
            return;
        }
    }
    // create new
    const auto element = editable_circuit_.layout().element(setting_handle.element_id);
    auto* widget = new ClockGeneratorDialog {parent_, AttributeSetter {this},
                                             element.attrs_clock_generator()};
    widget->setWindowFlags(Qt::Dialog);
    widget->setAttribute(Qt::WA_DeleteOnClose);

    // add to map
    try {
        const auto [it, inserted] = map_.emplace(widget, editable_circuit_.get_handle());
        if (!inserted) {
            throw_exception("could not insert widget into map");
        }
        it->second.value().add_logicitem(setting_handle.element_id);

        connect(widget, &QWidget::destroyed, this,
                &SettingWidgetRegistry::on_dialog_destroyed);
    } catch (std::exception&) {
        widget->deleteLater();
        throw;
    }

    widget->show();
}

auto SettingWidgetRegistry::close_all() -> void {
    for (auto&& [widget, _] : map_) {
        widget->deleteLater();
    }
    map_.clear();
}

auto SettingWidgetRegistry::get_element_id(QWidget* dialog) const -> element_id_t {
    if (const auto it = map_.find(dialog); it != map_.end()) {
        const auto& [widget, selection_handle] = *it;
        const auto& selected_items = selection_handle.value().selected_logic_items();

        if (selected_items.size() > 1) {
            throw_exception("unexpected selected items size in SettingWidgetRegistry");
        }
        if (selected_items.size() == 0) {
            return null_element;
        }

        return selected_items.front();
    }
    return null_element;
}

auto SettingWidgetRegistry::set_attributes(QWidget* widget,
                                           layout::attributes_clock_generator attrs)
    -> void {
    if (!widget) {
        throw_exception("widget sender cannot be nullptr");
    }

    const auto element_id = get_element_id(widget);
    if (!element_id) {
        widget->deleteLater();
        return;
    }

    editable_circuit_.set_attributes(element_id, std::move(attrs));
    if (parent_) {
        parent_->update();
    }
}

auto SettingWidgetRegistry::on_dialog_destroyed(QObject* object) -> void {
    auto* widget = reinterpret_cast<QWidget*>(object);
    if (!widget) {
        throw_exception("got non-widget object in dialog_destroyed");
    }
    map_.erase(widget);
}

auto SettingWidgetRegistry::on_cleanup_timeout() -> void {
    for (auto&& [widget, selection_handle] : map_) {
        if (selection_handle && selection_handle->selected_logic_items().empty()) {
            widget->deleteLater();
        }
    }
}

AttributeSetter::AttributeSetter(SettingWidgetRegistry* receiver)
    : receiver_ {receiver} {}

auto AttributeSetter::set_attributes(QWidget* sender,
                                     layout::attributes_clock_generator attrs) -> void {
    if (!sender) {
        throw_exception("sender cannot be nullptr");
    }

    if (receiver_) {
        receiver_->set_attributes(sender, attrs);
    } else {
        sender->deleteLater();
    }
}

//
// Clock Generator Dialog
//

DelayInput::DelayInput(QWidget* parent, QString text, delay_t initial_value,
                       double scale_)
    : QObject(parent), scale {scale_}, last_valid_delay {initial_value} {
    label = new QLabel(parent);
    label->setText(text);

    // TODO handle overflow

    layout = new QHBoxLayout();
    auto* line_edit = new QLineEdit(parent);
    auto* combo_box = new QComboBox(parent);

    line_edit->setValidator(&delay_validator);

    combo_box->addItem(tr("ns"), qint64 {1});
    combo_box->addItem(tr("µs"), qint64 {1'000});
    combo_box->addItem(tr("ms"), qint64 {1'000'000});

    const auto value_ns = initial_value.value / 1ns * scale;
    for (auto index : range(combo_box->count())) {
        const auto unit = combo_box->itemData(index).toLongLong();
        if (round_to<int64_t>(value_ns) >= int64_t {unit}) {
            combo_box->setCurrentIndex(index);
        }
    }
    const auto unit = combo_box->currentData().toLongLong();
    line_edit->setText(delay_validator.locale().toString(1.0 * value_ns / unit));

    layout->addWidget(line_edit);
    layout->addWidget(combo_box);

    delay_value = line_edit;
    delay_unit = combo_box;

    connect(delay_unit, &QComboBox::currentIndexChanged, this,
            &DelayInput::delay_unit_changed);

    connect(delay_value, &QLineEdit::textChanged, this, &DelayInput::value_changed);
    connect(delay_unit, &QComboBox::currentIndexChanged, this,
            &DelayInput::value_changed);

    delay_unit_changed();
}

auto DelayInput::value_changed() -> void {
    if (!delay_value || !delay_unit) {
        throw_exception("a pointer is not set in DelayInput");
    }

    if (delay_value->hasAcceptableInput()) {
        const auto value = delay_validator.locale().toDouble(delay_value->text());
        const auto unit = int64_t {delay_unit->currentData().toLongLong()};
        const auto period = delay_t {round_to<int64_t>(value * unit / scale) * 1ns};
        last_valid_delay = period;
    }
}

auto DelayInput::delay_unit_changed() -> void {
    const auto unit = int64_t {delay_unit->currentData().toLongLong()};

    if (unit == 1) {
        delay_validator.setDecimals(0);
    } else if (unit == 1'000) {
        delay_validator.setDecimals(3);
    } else if (unit == 1'000'000) {
        delay_validator.setDecimals(6);
    } else {
        throw_exception("unexpected unit");
    }

    // TODO fix overflow
    double max_ns = gsl::narrow_cast<double>(delay_t::max().value / 1ns) * scale;
    double min_ns = 1.0 * scale;
    delay_validator.setRange(min_ns / unit, max_ns / unit);
}

ClockGeneratorDialog::ClockGeneratorDialog(QWidget* parent, AttributeSetter setter,
                                           layout::attributes_clock_generator attrs)
    : QWidget {parent}, setter_ {setter} {
    setWindowTitle(tr("Clock Generator"));
    setWindowIcon(QIcon(get_icon_path(icon_t::setting_handle_clock_generator)));

    auto* layout = new QFormLayout(this);
    layout_ = layout;

    // Name
    {
        auto* label = new QLabel(this);
        label->setText(tr("Clock Name:"));
        auto* line_edit = new QLineEdit(this);
        line_edit->setText(QString::fromStdString(attrs.name));

        layout->addRow(label, line_edit);

        name_ = line_edit;
        connect(name_, &QLineEdit::textChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    // Is Symmetric
    {
        auto* check_box = new QCheckBox(this);
        check_box->setText(tr("Symmetric Period"));
        check_box->setChecked(attrs.is_symmetric);

        layout->addRow(nullptr, check_box);
        is_symmetric_ = check_box;

        connect(is_symmetric_, &QCheckBox::stateChanged, this,
                &ClockGeneratorDialog::update_row_visibility);
        connect(is_symmetric_, &QCheckBox::stateChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    // Time Symmetric
    {
        time_symmetric_ = new DelayInput(this, tr("Period:"), attrs.time_symmetric, 2.0);
        layout->addRow(time_symmetric_->label, time_symmetric_->layout);

        connect(time_symmetric_->delay_value, &QLineEdit::textChanged, this,
                &ClockGeneratorDialog::value_changed);
        connect(time_symmetric_->delay_unit, &QComboBox::currentIndexChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    // Time On
    {
        time_on_ = new DelayInput(this, tr("On Time:"), attrs.time_on, 1.0);
        layout->addRow(time_on_->label, time_on_->layout);

        connect(time_on_->delay_value, &QLineEdit::textChanged, this,
                &ClockGeneratorDialog::value_changed);
        connect(time_on_->delay_unit, &QComboBox::currentIndexChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    // Time Off
    {
        time_off_ = new DelayInput(this, tr("Off Time:"), attrs.time_off, 1.0);
        layout->addRow(time_off_->label, time_off_->layout);

        connect(time_off_->delay_value, &QLineEdit::textChanged, this,
                &ClockGeneratorDialog::value_changed);
        connect(time_off_->delay_unit, &QComboBox::currentIndexChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    // Simulation Controls
    {
        auto* check_box = new QCheckBox(this);
        check_box->setText(tr("Show Simulation Controls"));
        check_box->setChecked(attrs.show_simulation_controls);

        layout->addRow(nullptr, check_box);
        simulation_controls_ = check_box;
        connect(simulation_controls_, &QCheckBox::stateChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    update_row_visibility();
}

auto ClockGeneratorDialog::value_changed() -> void {
    if (!name_ || !is_symmetric_ || !time_symmetric_ || !time_on_ || !time_off_ ||
        !simulation_controls_) {
        throw_exception("a pointer is not set");
    }

    setter_.set_attributes(
        this, layout::attributes_clock_generator {
                  .name = name_->text().toStdString(),

                  .time_symmetric = time_symmetric_->last_valid_delay,
                  .time_on = time_on_->last_valid_delay,
                  .time_off = time_off_->last_valid_delay,

                  .is_symmetric = is_symmetric_->isChecked(),
                  .show_simulation_controls = simulation_controls_->isChecked(),
              });
}

auto ClockGeneratorDialog::update_row_visibility() -> void {
    const auto is_symmetric = is_symmetric_->isChecked();

    layout_->setRowVisible(time_symmetric_->label, is_symmetric);
    layout_->setRowVisible(time_on_->label, !is_symmetric);
    layout_->setRowVisible(time_off_->label, !is_symmetric);

    this->adjustSize();
}

//
// Mouse Setting Handle Logic
//

MouseSettingHandleLogic::MouseSettingHandleLogic(Args args) noexcept
    : widget_registry_ {args.widget_registry}, setting_handle_ {args.setting_handle} {}

auto MouseSettingHandleLogic::mouse_press(point_fine_t position) -> void {
    first_position_ = position;
}

auto MouseSettingHandleLogic::mouse_release(point_fine_t position) -> void {
    if (first_position_ &&  //
        is_colliding(setting_handle_, first_position_.value()) &&
        is_colliding(setting_handle_, position)) {
        widget_registry_.show_setting_dialog(setting_handle_);
    }
}
}  // namespace logicsim