#ifndef LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H
#define LOGIKSIM_TEST_EDITABLE_CIRCUIT_HANDLERS_TEST_HELPERS_H

#include "algorithm/fmt_join.h"
#include "component/editable_circuit/handler.h"
#include "component/editable_circuit/layout_index.h"
#include "component/editable_circuit/message_sender.h"
#include "layout.h"
#include "layout_message.h"
#include "logging.h"
#include "vocabulary/logicitem_definition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace logicsim {

// forward message to cache and store them
class MessageRecorder {
   public:
    MessageRecorder() = default;

    MessageRecorder(LayoutIndex &layout_index) : layout_index_ {&layout_index} {}

    inline auto submit(const editable_circuit::InfoMessage &message) -> void {
        messages_.push_back(message);
        if (layout_index_) {
            layout_index_->submit(message);
        }
    }

    inline auto messages() -> const std::vector<editable_circuit::InfoMessage> & {
        return messages_;
    }

    inline auto print() -> void {
        logicsim::print();
        logicsim::print(fmt_join("{}", messages_, "\n"));
        logicsim::print();
    }

   private:
    LayoutIndex *layout_index_ {nullptr};
    std::vector<editable_circuit::InfoMessage> messages_ {};
};

inline auto make_sender(MessageRecorder &recorder) -> editable_circuit::MessageSender {
    const auto callback = [recorder = &recorder](const auto &message) {
        recorder->submit(message);
    };
    return editable_circuit::MessageSender {callback};
}

struct HandlerSetup {
    Layout &layout;
    LayoutIndex cache;
    MessageRecorder recorder;
    editable_circuit::MessageSender sender;
    editable_circuit::State state;

    HandlerSetup(Layout &layout_)
        : layout {layout_},
          cache {layout_},
          recorder {cache},
          sender {make_sender(recorder)},
          state {layout_, sender, cache} {
        validate();
    }

    auto validate() const -> void {
        // if (LayoutIndex {layout} != cache) [[unlikely]] {
        //     throw std::runtime_error("layout index is out of sync");
        // }

        const auto index = LayoutIndex {layout};

        if (index != cache) [[unlikely]] {
            if (index.logicitem_input_index() != cache.logicitem_input_index()) {
                throw std::runtime_error("logicitem_input_index is out of sync");
            }
            if (index.logicitem_output_index() != cache.logicitem_output_index()) {
                throw std::runtime_error("logicitem_output_index is out of sync");
            }
            if (index.wire_input_index() != cache.wire_input_index()) {
                throw std::runtime_error("wire_input_index is out of sync");
            }
            if (index.wire_output_index() != cache.wire_output_index()) {
                throw std::runtime_error("wire_output_index is out of sync");
            }
            if (index.collision_index() != cache.collision_index()) {
                throw std::runtime_error("collision_index is out of sync");
            }
            if (index.selection_index() != cache.selection_index()) {
                throw std::runtime_error("selection_index is out of sync");
            }

            throw std::runtime_error("WARNING Index is out of sync");
        }
    }
};

struct SenderSetup {
    MessageRecorder recorder;
    editable_circuit::MessageSender sender;

    SenderSetup() : recorder {}, sender {make_sender(recorder)} {}
};

inline auto add_and_element(Layout &layout, display_state_t display_type,
                            connection_count_t input_count = connection_count_t {3},
                            point_t position = point_t {0, 0}) -> logicitem_id_t {
    const auto definition = LogicItemDefinition {
        .logicitem_type = LogicItemType::and_element,

        .input_count = input_count,
        .output_count = connection_count_t {1},
        .orientation = orientation_t::right,
    };
    return layout.logic_items().add(definition, position, display_type);
}

inline auto assert_logicitem_count(const Layout &layout, std::size_t count) -> void {
    ASSERT_EQ(layout.logic_items().size(), count);
}

inline auto assert_logicitem_equal(
    const Layout &layout, logicitem_id_t logicitem_id,
    connection_count_t input_count = connection_count_t {3},
    point_t position = point_t {0, 0}) -> void {
    ASSERT_EQ(layout.logic_items().input_count(logicitem_id), input_count);
    ASSERT_EQ(layout.logic_items().position(logicitem_id), position);
}

}  // namespace logicsim

#endif