#include "gui/widget/setting_dialog.h"

#include "gui/qt/path_conversion.h"

#include "core/algorithm/range.h"
#include "core/algorithm/round.h"
#include "core/resource.h"
#include "core/validate_definition_logicitem.h"
#include "core/vocabulary/decoration_definition.h"
#include "core/vocabulary/delay.h"
#include "core/vocabulary/logicitem_definition.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QRegularExpression>

#include <stdexcept>

namespace logicsim {

//
// Setting Dialog
//

SettingDialog::SettingDialog(QWidget* parent, selection_id_t selection_id)
    : QWidget {parent}, selection_id_ {selection_id} {
    setWindowFlags(Qt::Dialog);
    setAttribute(Qt::WA_DeleteOnClose);
}

void SettingDialog::emit_attributes_changed(const SettingAttributes& attributes) {
    Q_EMIT attributes_changed(selection_id_, attributes);
}

//
// Delay Input
//

DelayInput::DelayInput(QWidget* parent, const QString& text, delay_t initial_value,
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

    const auto value_ns = gsl::narrow<double>(initial_value.count_ns()) * scale;
    for (auto index : range(combo_box->count())) {
        const auto unit = combo_box->itemData(index).toLongLong();
        if (round_to<int64_t>(value_ns) >= int64_t {unit}) {
            combo_box->setCurrentIndex(index);
        }
    }
    const auto unit = gsl::narrow<double>(combo_box->currentData().toLongLong());
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
    Expects(delay_value != nullptr);
    Expects(delay_unit != nullptr);

    if (delay_value->hasAcceptableInput()) {
        const auto value = delay_validator.locale().toDouble(delay_value->text());
        const auto unit = gsl::narrow<double>(delay_unit->currentData().toLongLong());
        const auto period = delay_t {round_to<int64_t>(value * unit / scale) * 1ns};
        last_valid_delay = period;
    }
}

auto DelayInput::delay_unit_changed() -> void {
    Expects(delay_unit != nullptr);

    const auto unit = gsl::narrow<double>(delay_unit->currentData().toLongLong());

    if (unit == 1) {
        delay_validator.setDecimals(0);
    } else if (unit == 1'000) {
        delay_validator.setDecimals(3);
    } else if (unit == 1'000'000) {
        delay_validator.setDecimals(6);
    } else {
        throw std::runtime_error("unexpected unit");
    }

    // stored value range
    const auto min_time = clock_generator_min_time();
    const auto max_time = clock_generator_max_time();

    const double min_ns = gsl::narrow<double>(min_time.count_ns()) * scale;
    const double max_ns = gsl::narrow<double>(max_time.count_ns()) * scale;
    delay_validator.setRange(min_ns / unit, max_ns / unit);
}

//
// Clock Generator Dialog
//

ClockGeneratorDialog::ClockGeneratorDialog(QWidget* parent, selection_id_t selection_id,
                                           const attributes_clock_generator_t& attrs)
    : SettingDialog {parent, selection_id} {
    setWindowTitle(tr("Clock Generator"));
    const auto path = get_icon_path(icon_t::setting_handle_clock_generator);
    setWindowIcon(QIcon(to_qt(path)));

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
    Expects(name_ != nullptr);
    Expects(time_symmetric_ != nullptr);
    Expects(time_on_ != nullptr);
    Expects(time_off_ != nullptr);

    Expects(is_symmetric_ != nullptr);
    Expects(simulation_controls_ != nullptr);

    emit_attributes_changed(attributes_clock_generator_t {
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
// Text Element Dialog
//

TextElementDialog::TextElementDialog(QWidget* parent, selection_id_t selection_id,
                                     const attributes_text_element_t& attrs)
    : SettingDialog {parent, selection_id} {
    setWindowTitle(tr("Text Element"));
    const auto path = get_icon_path(icon_t::setting_handle_text_element);
    setWindowIcon(QIcon(to_qt(path)));

    auto* layout = new QFormLayout(this);

    // Text
    {
        auto* label = new QLabel(this);
        label->setText(tr("Text:"));
        auto* line_edit = new QLineEdit(this);
        line_edit->setText(QString::fromStdString(attrs.text));

        layout->addRow(label, line_edit);

        text_ = line_edit;
        connect(text_, &QLineEdit::textChanged, this, &TextElementDialog::value_changed);
    }

    Ensures(text_ != nullptr);
}

auto TextElementDialog::value_changed() -> void {
    Expects(text_ != nullptr);

    emit_attributes_changed(attributes_text_element_t {
        .text = text_->text().toStdString(),
    });
}

}  // namespace logicsim
