#include "setting_handle.h"

#include "collision.h"
#include "editable_circuit/editable_circuit.h"
#include "editable_circuit/selection.h"
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
            constexpr auto overdraw = defaults::logic_item_body_overdraw;
            constexpr auto handle_size = defaults::setting_handle_size;
            constexpr auto margin = defaults::setting_handle_margin;

            const auto width = 3.0;
            const auto height = 2.0;

            return setting_handle_t {
                .position = transform(element.position(), element.orientation(),
                                      point_fine_t {
                                          width - handle_size / 2.0 - margin,
                                          height + overdraw - handle_size / 2.0 - margin,
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
    : parent_ {parent}, editable_circuit_ {editable_circuit} {}

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
    auto* widget = new ClockGeneratorDialog {parent_, *this, element};
    widget->setWindowFlags(Qt::Dialog);
    widget->setAttribute(Qt::WA_DeleteOnClose);

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
        disconnect(widget, &QWidget::destroyed, this,
                   &SettingWidgetRegistry::on_dialog_destroyed);
        delete widget;
    }
    map_.clear();
}

auto SettingWidgetRegistry::element_id(QWidget* dialog) const -> element_id_t {
    if (const auto it = map_.find(dialog); it != map_.end()) {
        const auto& selection_handle = it->second;
        const auto& selected_items = selection_handle.value().selected_logic_items();

        if (selected_items.size() != 1) {
            throw_exception("unexpected selected items size in SettingWidgetRegistry");
        }

        return selected_items.front();
    }
    throw_exception("dialog is not registered in SettingWidgetRegistry");
}

auto SettingWidgetRegistry::element(QWidget* dialog) const -> layout::ConstElement {
    return editable_circuit_.layout().element(element_id(dialog));
}

auto SettingWidgetRegistry::set_attributes(QWidget* dialog,
                                           layout::attributes_clock_generator attrs)
    -> void {
    editable_circuit_.set_attributes(element_id(dialog), std::move(attrs));

    if (parent_) {
        parent_->update();
    }
}

auto SettingWidgetRegistry::on_dialog_destroyed(QObject* object) -> void {
    auto* widget = reinterpret_cast<QWidget*>(object);
    if (!widget) {
        throw_exception("got non-widget object in dialog_destroyed");
    }
    if (map_.erase(widget) != 1) {
        throw_exception("did not find widget marked for deletion");
    }
    print("object erased", map_.size());
}

//
// Clock Generator Dialog
//

PeriodInput::PeriodInput(QWidget* parent, QString text, delay_t initial_value,
                         double scale_)
    : QObject(parent), scale {scale_}, last_valid_period {initial_value} {
    label = new QLabel(parent);
    label->setText(text);

    layout = new QHBoxLayout();
    auto* line_edit = new QLineEdit(parent);
    auto* combo_box = new QComboBox(parent);

    line_edit->setValidator(&period_validator);

    combo_box->addItem(tr("ns"), qint64 {1});
    combo_box->addItem(tr("µs"), qint64 {1'000});
    combo_box->addItem(tr("ms"), qint64 {1'000'000});

    const auto value_ns = initial_value.value / 1ns;
    for (auto index : range(combo_box->count())) {
        const auto unit = combo_box->itemData(index).toLongLong();
        if (value_ns >= unit) {
            combo_box->setCurrentIndex(index);
        }
    }
    const auto unit = combo_box->currentData().toLongLong();
    line_edit->setText(period_validator.locale().toString(scale * value_ns / unit));

    layout->addWidget(line_edit);
    layout->addWidget(combo_box);

    period_value = line_edit;
    period_unit = combo_box;

    connect(period_unit, &QComboBox::currentIndexChanged, this,
            &PeriodInput::period_unit_changed);

    connect(period_value, &QLineEdit::textChanged, this, &PeriodInput::value_changed);
    connect(period_unit, &QComboBox::currentIndexChanged, this,
            &PeriodInput::value_changed);

    period_unit_changed();
}

auto PeriodInput::value_changed() -> void {
    if (!period_value || !period_unit) {
        throw_exception("a pointer is not set in PeriodInput");
    }

    if (period_value->hasAcceptableInput()) {
        const auto value = period_validator.locale().toDouble(period_value->text());
        const auto unit = int64_t {period_unit->currentData().toLongLong()};
        const auto period = delay_t {round_to<int64_t>(value * unit / scale) * 1ns};
        last_valid_period = period;
    }
}

auto PeriodInput::period_unit_changed() -> void {
    const auto unit = int64_t {period_unit->currentData().toLongLong()};

    if (unit == 1) {
        period_validator.setDecimals(0);
    } else if (unit == 1'000) {
        period_validator.setDecimals(3);
    } else if (unit == 1'000'000) {
        period_validator.setDecimals(6);
    } else {
        throw_exception("unexpected unit");
    }

    double max_ns = delay_t::max().value / 1ns * scale;
    double min_ns = 1.0 * scale;
    period_validator.setRange(min_ns / unit, max_ns / unit);
}

ClockGeneratorDialog::ClockGeneratorDialog(QWidget* parent,
                                           SettingWidgetRegistry& widget_registry,
                                           layout::ConstElement element)
    : QWidget(parent), widget_registry_ {widget_registry} {
    setWindowTitle(tr("Clock Generator"));
    setWindowIcon(QIcon(get_icon_path(icon_t::setting_handle_clock_generator)));

    const auto& attrs = element.attrs_clock_generator();

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

    // Symmetric Period
    {
        auto* check_box = new QCheckBox(this);
        check_box->setText(tr("Symmetric Period"));
        check_box->setChecked(attrs.symmetric_period);

        layout->addRow(nullptr, check_box);
        symmetric_period_ = check_box;

        connect(symmetric_period_, &QCheckBox::stateChanged, this,
                &ClockGeneratorDialog::update_row_visibility);
        connect(symmetric_period_, &QCheckBox::stateChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    // Symmetric Period
    {
        period_ = new PeriodInput(this, tr("Period:"), attrs.period, 2.0);
        layout->addRow(period_->label, period_->layout);

        connect(period_->period_value, &QLineEdit::textChanged, this,
                &ClockGeneratorDialog::value_changed);
        connect(period_->period_unit, &QComboBox::currentIndexChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    // On Period
    {
        period_on_ = new PeriodInput(this, tr("On Time:"), attrs.period_on, 1.0);
        layout->addRow(period_on_->label, period_on_->layout);

        connect(period_on_->period_value, &QLineEdit::textChanged, this,
                &ClockGeneratorDialog::value_changed);
        connect(period_on_->period_unit, &QComboBox::currentIndexChanged, this,
                &ClockGeneratorDialog::value_changed);
    }

    // Off Period
    {
        period_off_ = new PeriodInput(this, tr("Off Time:"), attrs.period_off, 1.0);
        layout->addRow(period_off_->label, period_off_->layout);

        connect(period_off_->period_value, &QLineEdit::textChanged, this,
                &ClockGeneratorDialog::value_changed);
        connect(period_off_->period_unit, &QComboBox::currentIndexChanged, this,
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
    if (!name_ || !symmetric_period_ || !period_ || !period_on_ || !period_off_ ||
        !simulation_controls_) {
        throw_exception("a pointer is not set");
    }

    widget_registry_.set_attributes(
        this, layout::attributes_clock_generator {
                  .name = name_->text().toStdString(),
                  .symmetric_period = symmetric_period_->isChecked(),
                  .period = period_->last_valid_period,
                  .period_on = period_on_->last_valid_period,
                  .period_off = period_off_->last_valid_period,
                  .show_simulation_controls = simulation_controls_->isChecked(),
              });
}

auto ClockGeneratorDialog::update_row_visibility() -> void {
    const auto symmetric_period = symmetric_period_->isChecked();

    layout_->setRowVisible(period_->label, false);
    layout_->setRowVisible(period_on_->label, false);
    layout_->setRowVisible(period_off_->label, false);

    layout_->setRowVisible(period_->label, symmetric_period);
    layout_->setRowVisible(period_on_->label, !symmetric_period);
    layout_->setRowVisible(period_off_->label, !symmetric_period);

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
    if (first_position_ && is_colliding(setting_handle_, first_position_.value()) &&
        is_colliding(setting_handle_, position)) {
        widget_registry_.show_setting_dialog(setting_handle_);
    }
}

}  // namespace logicsim