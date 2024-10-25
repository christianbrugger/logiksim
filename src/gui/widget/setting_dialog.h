#ifndef LOGICSIM_WIDGET_SETTING_DIALOG_H
#define LOGICSIM_WIDGET_SETTING_DIALOG_H

#include "core/vocabulary/decoration_definition.h"
#include "core/vocabulary/delay.h"
#include "core/vocabulary/font_style.h"
#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/selection_id.h"
#include "core/vocabulary/setting_attribute.h"

#include <ankerl/unordered_dense.h>

#include <QAbstractButton>
#include <QDoubleValidator>
#include <QWidget>

#include <vector>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLayout;
class QLineEdit;
class QCloseEvent;

namespace logicsim {

struct delay_t;
struct FontStyleInfo;

//
// Setting Dialog
//

class SettingDialog : public QWidget {
    Q_OBJECT

   public:
    explicit SettingDialog(QWidget* parent, selection_id_t selection_id);

   public:
    Q_SIGNAL void attributes_changed(selection_id_t selection_id,
                                     const SettingAttributes& attributes);

   protected:
    Q_SLOT void emit_attributes_changed(const SettingAttributes& attributes);

   private:
    selection_id_t selection_id_;
};

//
// Clock Generator Dialog
//

class DelayInput : public QObject {
    Q_OBJECT

   public:
    explicit DelayInput(QWidget* parent, const QString& text, delay_t initial_value,
                        double scale_);

    // TODO change names to better reflect slot nature
    auto value_changed() -> void;
    auto delay_unit_changed() -> void;

    double scale;
    delay_t last_valid_delay;

    // TODO make private
    // TODO initialize with {}
    // TODO use not null and remove Expects

    QLineEdit* delay_value;
    QComboBox* delay_unit;
    QDoubleValidator delay_validator;

    QLabel* label;
    QLayout* layout;
};

class ClockGeneratorDialog : public SettingDialog {
    Q_OBJECT

   public:
    explicit ClockGeneratorDialog(QWidget* parent, selection_id_t selection_id,
                                  const attributes_clock_generator_t& attrs);

   private:
    auto value_changed() -> void;
    auto update_row_visibility() -> void;

   private:
    QFormLayout* layout_;

    QLineEdit* name_;
    DelayInput* time_symmetric_;
    DelayInput* time_on_;
    DelayInput* time_off_;

    QCheckBox* is_symmetric_;
    QCheckBox* simulation_controls_;
};

class TextElementDialog : public SettingDialog {
    Q_OBJECT

   public:
    explicit TextElementDialog(QWidget* parent, selection_id_t selection_id,
                               const attributes_text_element_t& attrs);

   private:
    [[nodiscard]] static auto get_style_button_infos() -> std::vector<FontStyleInfo>;
    [[nodiscard]] auto get_selected_font_style() const -> FontStyle;

    auto value_changed() -> void;

   private:
    QLineEdit* text_;

    using font_style_map_type = ankerl::unordered_dense::map<FontStyle, QAbstractButton*>;
    font_style_map_type font_style_buttons_ {};
};

}  // namespace logicsim

#endif
