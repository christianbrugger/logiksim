#include <catch2/catch_test_macros.hpp>

#include <gui/qt/path_conversion.h>

namespace logicsim {

TEST_CASE("back and forth simple path", "[qt/path_conversion]") {
    const auto path = std::filesystem::path("abc");

    REQUIRE(to_path(to_qt(path)) == path);
}

}  // namespace logicsim
