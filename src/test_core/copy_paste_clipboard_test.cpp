
#include "core/copy_paste_clipboard.h"

#include "core/editable_circuit.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace {

[[nodiscard]] auto get_clipboard_test_circuit() -> Layout {
    auto rng = get_random_number_generator(1);
    auto editable_circuit = EditableCircuit {};
    add_example(rng, editable_circuit);

    EXPECT_TRUE(editable_circuit.layout().logicitems().size() > 0);
    EXPECT_TRUE(editable_circuit.layout().decorations().size() > 0);
    return editable_circuit.extract_layout();
}

}  // namespace

TEST(CopyPasteClipboard, SamePosition) {
    const auto layout = get_clipboard_test_circuit();
    const auto copy_position = point_t {1, 1};
    const auto paste_position = point_t {1, 1};

    // copy
    const auto text = layout_to_clipboard_text(layout, copy_position);
    EXPECT_TRUE(guess_save_format(text) == SaveFormat::base64_gzip);

    // parse
    const auto load_result = parse_clipboard_text(text);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_TRUE(load_result->save_position() == copy_position);

    // paste
    auto editable_circuit = EditableCircuit {};
    const auto result =
        insert_clipboard_data(editable_circuit, load_result.value(), paste_position);
    EXPECT_TRUE(result.is_colliding == false);

    // equal
    EXPECT_TRUE(are_normalized_equal(layout, editable_circuit.layout()));
}

}  // namespace logicsim
