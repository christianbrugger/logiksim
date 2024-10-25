#include "gui/widget/setting_dialog.h"

#include "gui/format/qt_type.h"
#include "gui/qt/path_conversion.h"
#include "gui/qt/svg_icon_engine.h"

#include "core/algorithm/overload.h"
#include "core/algorithm/range.h"
#include "core/algorithm/round.h"
#include "core/concept/input_range.h"
#include "core/file.h"
#include "core/logging.h"
#include "core/resource.h"
#include "core/validate_definition_logicitem.h"
#include "core/vocabulary/decoration_definition.h"
#include "core/vocabulary/delay.h"
#include "core/vocabulary/font_style.h"
#include "core/vocabulary/logicitem_definition.h"

#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/single.hpp>
#include <range/v3/view/transform.hpp>

#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QRegularExpression>
#include <QToolButton>

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

struct FontStyleInfo {
    icon_t icon;
    QString tooltip;

    FontStyle font_style;

    bool is_bold {false};
    bool is_italic {false};
};

auto TextElementDialog::get_style_button_infos() -> std::vector<FontStyleInfo> {
    return {
        FontStyleInfo {
            .icon = icon_t::text_style_regular,
            .tooltip = tr("Regular"),
            .font_style = FontStyle::regular,
        },
        FontStyleInfo {
            .icon = icon_t::text_style_bold,
            .tooltip = tr("Bold"),
            .font_style = FontStyle::bold,
            .is_bold = true,
        },
        FontStyleInfo {
            .icon = icon_t::text_style_italic,
            .tooltip = tr("Italic"),
            .font_style = FontStyle::italic,
            .is_italic = true,
        },
        FontStyleInfo {
            .icon = icon_t::text_style_monospace,
            .tooltip = tr("Monospace"),
            .font_style = FontStyle::monospace,
        },
    };
}

struct AlignmentInfo {
    icon_t icon;
    QString tooltip;

    HTextAlignment alignment;
};

auto TextElementDialog::get_alignment_button_infos() -> std::vector<AlignmentInfo> {
    return {
        AlignmentInfo {
            .icon = icon_t::text_alignment_horizontal_left,
            .tooltip = tr("Left"),
            .alignment = HTextAlignment::left,
        },
        AlignmentInfo {
            .icon = icon_t::text_alignment_horizontal_center,
            .tooltip = tr("Center"),
            .alignment = HTextAlignment::center,
        },
        AlignmentInfo {
            .icon = icon_t::text_alignment_horizontal_right,
            .tooltip = tr("Right"),
            .alignment = HTextAlignment::right,
        },
    };
}

namespace {

// TODO move somewhere else
[[nodiscard]] auto get_buttons_max_size(input_range_of<QAbstractButton*> auto&& buttons)
    -> QSize {
    const auto to_size_hint = [](const QAbstractButton* button) -> QSize {
        Expects(button != nullptr);
        return button->sizeHint();
    };

    const auto size_union = [](const QSize& a, const QSize& b) -> QSize {
        return a.expandedTo(b);
    };

    const auto size_hints = ranges::views::transform(buttons, to_size_hint);
    return ranges::accumulate(size_hints, QSize {}, size_union);
}

// TODO move somewhere else
[[nodiscard]] auto to_squared_size(QSize size) -> QSize {
    const auto max_extend = qMax(size.width(), size.height());
    return QSize {max_extend, max_extend};
}

// TODO move somewhere else
[[nodiscard]] auto get_buttons_max_extend(input_range_of<QAbstractButton*> auto&& buttons)
    -> QSize {
    return to_squared_size(get_buttons_max_size(buttons));
}

// TODO move somewhere else
auto set_buttons_size(input_range_of<QAbstractButton*> auto&& buttons,
                      QSize size) -> void {
    for (QAbstractButton* button : buttons) {
        Expects(button != nullptr);
        button->setFixedSize(size);
    }
}

// TODO move somewhere else
auto set_buttons_to_equal_squares(input_range_of<QAbstractButton*> auto&& buttons)
    -> void {
    set_buttons_size(buttons, get_buttons_max_extend(buttons));
}

// TODO move somewhere else
auto set_font_size_ratio(QWidget* widget, double ratio) -> void {
    Expects(widget != nullptr);

    auto font = widget->font();
    font.setPointSizeF(font.pointSizeF() * ratio);
    widget->setFont(font);
};

auto set_button_icon(QAbstractButton* button, icon_t icon, QSize size) -> void {
    Expects(button != nullptr);

    const auto qt_icon = QIcon(to_qt(get_icon_path(icon)));
    button->setIcon(qt_icon);
    button->setIconSize(size);
}

constexpr inline auto svg_color_template = R"(
<svg
  xmlns="http://www.w3.org/2000/svg"
  width="24"
  height="24"
  viewBox="0 0 24 24"
>
  <rect x="2" y="2" width="20" height="20" rx="2" fill="#{:02X}{:02X}{:02X}"/>
</svg>
)";

auto create_icon_from_color(color_t color) -> QIcon {
    const auto svg = fmt::format(svg_color_template, color.r(), color.g(), color.b());
    return QIcon {new SvgIconEngine {svg}};
}

}  // namespace

TextElementDialog::TextElementDialog(QWidget* parent, selection_id_t selection_id,
                                     const attributes_text_element_t& attrs)
    : SettingDialog {parent, selection_id} {
    setWindowTitle(tr("Text Element"));
    const auto path = get_icon_path(icon_t::dialog_text_element);
    setWindowIcon(QIcon(to_qt(path)));

    constexpr auto icon_size = QSize {18, 18};
    constexpr auto text_size_ratio = 1.1;
    constexpr auto text_margins = 1;

    auto* layout = new QFormLayout(this);

    // Text
    {
        auto* label = new QLabel(this);
        label->setText(tr("Text:"));
        auto* line_edit = new QLineEdit(this);
        layout->addRow(label, line_edit);

        line_edit->setText(QString::fromStdString(attrs.text));
        set_font_size_ratio(line_edit, text_size_ratio);
        line_edit->setTextMargins(text_margins, text_margins, text_margins, text_margins);

        text_ = line_edit;
        connect(text_, &QLineEdit::textChanged, this, &TextElementDialog::value_changed);
    }

    layout->addItem(new QSpacerItem {0, 2});

    // Font Style
    {
        auto* label = new QLabel(this);
        label->setText(tr("Style:"));
        auto* row_layout = new QHBoxLayout {};
        layout->addRow(label, row_layout);

        const auto infos = get_style_button_infos();
        auto* group = new QButtonGroup {this};

        for (const auto& info : infos) {
            auto* button = new QToolButton {this};
            set_button_icon(button, info.icon, icon_size);
            button->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
            button->setCheckable(true);
            button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            button->setToolTip(info.tooltip);

            auto font = button->font();
            font.setBold(info.is_bold);
            font.setItalic(info.is_italic);
            button->setFont(font);

            group->addButton(button);
            row_layout->addWidget(button, 1);

            font_style_buttons_[info.font_style] = button;
            connect(button, &QToolButton::clicked, this,
                    &TextElementDialog::value_changed);
        }

        row_layout->addStretch(1);

        // set active button
        font_style_buttons_[attrs.font_style]->setChecked(true);
    }

    // Horizontal Alignment
    {
        auto* label = new QLabel(this);
        label->setText(tr("Alignment:"));
        auto* row_layout = new QHBoxLayout {};
        layout->addRow(label, row_layout);

        const auto infos = get_alignment_button_infos();
        auto* group = new QButtonGroup {this};

        for (const auto& info : infos) {
            auto* button = new QToolButton {this};
            set_button_icon(button, info.icon, icon_size);
            button->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
            button->setCheckable(true);
            button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            button->setToolTip(info.tooltip);

            group->addButton(button);
            row_layout->addWidget(button, 1);

            alignment_buttons_[info.alignment] = button;
            connect(button, &QToolButton::clicked, this,
                    &TextElementDialog::value_changed);
        }

        row_layout->addStretch(1);

        // set active button
        alignment_buttons_[attrs.horizontal_alignment]->setChecked(true);
    }

    // Color
    {
        auto* label = new QLabel {this};
        label->setText(tr("Color:"));
        auto* button = new QToolButton {this};
        layout->addRow(label, button);

        button->setIcon(create_icon_from_color(attrs.text_color));
        button->setIconSize(icon_size);
        button->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);

        connect(button, &QToolButton::clicked, this,
                &TextElementDialog::on_color_button_clicked);

        color_button_ = button;
        text_color_ = attrs.text_color;
    }

    // equal button size
    auto buttons = ranges::views::concat(font_style_buttons_ | ranges::views::values,
                                         alignment_buttons_ | ranges::views::values,
                                         ranges::views::single(color_button_));
    set_buttons_to_equal_squares(buttons);

    resize(400, 50);
    Ensures(text_ != nullptr);
}

auto TextElementDialog::get_selected_font_style() const -> FontStyle {
    for (const auto& [style, button] : font_style_buttons_) {
        if (button->isChecked()) {
            return style;
        }
    }
    return attributes_text_element_t {}.font_style;
}

auto TextElementDialog::get_selected_alignment() const -> HTextAlignment {
    for (const auto& [alignment, button] : alignment_buttons_) {
        if (button->isChecked()) {
            return alignment;
        }
    }
    return attributes_text_element_t {}.horizontal_alignment;
}

auto TextElementDialog::on_color_button_clicked() -> void {
    // TODO put somewhere else
    const auto initial = QColor {
        gsl::narrow<int>(text_color_.r()),
        gsl::narrow<int>(text_color_.g()),
        gsl::narrow<int>(text_color_.b()),
    };
    const auto res = QColorDialog::getColor(initial);

    if (res.isValid()) {
        const auto color = color_t {
            gsl::narrow<uint32_t>(res.red()),
            gsl::narrow<uint32_t>(res.green()),
            gsl::narrow<uint32_t>(res.blue()),
        };

        color_button_->setIcon(create_icon_from_color(color));
        text_color_ = color;
        value_changed();
    }
}

auto TextElementDialog::value_changed() -> void {
    Expects(text_ != nullptr);

    emit_attributes_changed(attributes_text_element_t {
        .text = text_->text().toStdString(),
        .horizontal_alignment = get_selected_alignment(),
        .font_style = get_selected_font_style(),
        .text_color = text_color_,
    });
}

}  // namespace logicsim
