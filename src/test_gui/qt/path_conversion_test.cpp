#include "gui/qt/path_conversion.h"

#include "core/algorithm/path_conversion.h"
#include "core/file.h"

#include <catch2/catch_test_macros.hpp>

#include <QDir>

#include <filesystem>

namespace logicsim {

//
// Conversion Only
//

TEST_CASE("back and forth simple path", "[qt/path_conversion]") {
    const auto path = std::filesystem::path {"regular"};

    REQUIRE(to_path(to_qt(path)) == path);
}

TEST_CASE("back and forth utf-8 path", "[qt/path_conversion]") {
    // Snowman Emoji
    const auto path = std::filesystem::path {std::u8string {u8"snowman_\u2603"}};

    REQUIRE(to_path(to_qt(path)) == path);
}

TEST_CASE("back and forth utf-8 high path", "[qt/path_conversion]") {
    // Musical Symbol G Clef
    const auto path = std::filesystem::path {std::u8string {u8"musical_\U0001D11E"}};

    REQUIRE(to_path(to_qt(path)) == path);
}

//
// Use File API
//

namespace {

auto write_file_qt(const std::filesystem::path& orig_path,
                   const std::string& content) -> void {
    const auto qt_path = to_qt(orig_path);

    QDir {}.remove(qt_path);
    REQUIRE(!QFileInfo {qt_path}.isFile());
    const auto success = save_file(orig_path, content);
    REQUIRE(success);
    REQUIRE(QFileInfo {qt_path}.isFile());
    REQUIRE(QDir {}.exists(qt_path));
}

auto read_file_qt(const QString& qt_path, const std::string& content) -> void {
    const auto std_path = to_path(qt_path);

    const auto text = load_file(std_path);
    REQUIRE(text.has_value());
    REQUIRE(text.value() == content);
}

}  // namespace

TEST_CASE("read write simple path", "[qt/path_conversion]") {
    const auto path =
        std::filesystem::path {std::u8string {u8"unittest_qt_file_regular"}};

    write_file_qt(path, "test");
    read_file_qt(to_qt(path), "test");
}

TEST_CASE("read write utf-8 path", "[qt/path_conversion]") {
    // Snowman Emoji
    const auto path =
        std::filesystem::path {std::u8string {u8"unittest_qt_file_snowman_\u2603"}};

    write_file_qt(path, "test");
    read_file_qt(to_qt(path), "test");
}

TEST_CASE("read write utf-8 high path", "[qt/path_conversion]") {
    // Musical Symbol G Clef
    const auto path =
        std::filesystem::path {std::u8string {u8"unittest_qt_file_musical_\U0001D11E"}};

    write_file_qt(path, "test");
    read_file_qt(to_qt(path), "test");
}

//
// Windows Specific
//

#ifdef _WIN64

TEST_CASE("back and forth invalid surrogates", "[qt/path_conversion]") {
    // Unmatched suggorage pair
    const auto orig_path = std::filesystem::path {std::wstring {L"file_invalid_\xD800"}};
    // not representable as utf-8
    REQUIRE(!path_to_utf8(orig_path).has_value());

    const auto qt_path = to_qt(orig_path);
    const auto std_path = to_path(qt_path);

    REQUIRE(std_path == orig_path);
}

TEST_CASE("read write invalid surrogates", "[qt/path_conversion]") {
    // Unmatched suggorage pair
    const auto path =
        std::filesystem::path {std::wstring {L"unittest_qt_file_invalid_\xD800"}};
    // not representable as utf-8
    REQUIRE(!path_to_utf8(path).has_value());

    write_file_qt(path, "test");
    read_file_qt(to_qt(path), "test");
}

#endif

}  // namespace logicsim
