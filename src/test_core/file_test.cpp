#include "file.h"

#include "algorithm/path_conversion.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

namespace {

auto write_read_file(const std::filesystem::path& path) -> void {
    const auto content = std::string {"test"};

    std::filesystem::remove(path);
    ASSERT_EQ(std::filesystem::is_regular_file(path), false);
    const auto success = save_file(path, content);
    ASSERT_EQ(success, true);
    ASSERT_EQ(std::filesystem::is_regular_file(path), true);

    const auto text = load_file(path);
    ASSERT_EQ(text.has_value(), true);
    EXPECT_EQ(text.value(), content);
}

}  // namespace

TEST(File, RegularFile) {
    const auto path = std::filesystem::path {std::u8string {u8"unittest_file_regular"}};

    write_read_file(path);
}

TEST(File, Utf8File) {
    // Snowman Emoji
    const auto path =
        std::filesystem::path {std::u8string {u8"unittest_file_snowman_\u2603"}};

    write_read_file(path);
}

TEST(File, Utf8FileHigh) {
    // Musical Symbol G Clef
    const auto path =
        std::filesystem::path {std::u8string {u8"unittest_file_musical_\U0001D11E"}};

    write_read_file(path);
}

#ifdef _WIN64

TEST(File, WindowsInvalidSurrogates) {
    // Unmatched suggorage pair
    const auto path =
        std::filesystem::path {std::wstring {L"unittest_file_invalid_\xD800"}};

    // not representable as utf-8
    EXPECT_EQ(path_to_utf8(path).has_value(), false);

    write_read_file(path);
}

#endif

}  // namespace logicsim
