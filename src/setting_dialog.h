#ifndef LOGICSIM_SETTING_DIALOG_H
#define LOGICSIM_SETTING_DIALOG_H

#include "vocabulary/delay.h"
#include "vocabulary/selection_id.h"
#include "vocabulary/setting_attribute.h"

#include <QDoubleValidator>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLayout;
class QLineEdit;
class QCloseEvent;

namespace logicsim {

struct delay_t;
struct attributes_clock_generator_t;
struct LogicItemDefinition;

//
// Setting Dialog
//

class SettingDialog : public QWidget {
    Q_OBJECT

   public:
    explicit SettingDialog(QWidget* parent, selection_id_t selection_id);

   public:
    Q_SIGNAL void attributes_changed(selection_id_t selection_id,
                                     SettingAttributes attributes);

   protected:
    Q_SLOT void emit_attributes_changed(SettingAttributes attributes);

   private:
    selection_id_t selection_id_;
};

//
// Clock Generator Dialog
//

class DelayInput : public QObject {
    Q_OBJECT

   public:
    explicit DelayInput(QWidget* parent, QString text, delay_t initial_value,
                        double scale_);

    auto value_changed() -> void;
    auto delay_unit_changed() -> void;

    double scale;
    delay_t last_valid_delay;

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
                                  attributes_clock_generator_t attrs);

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

}  // namespace logicsim

#endif
