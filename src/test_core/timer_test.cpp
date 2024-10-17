
#include "core/timer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace logicsim {

TEST(Timer, NoLogging) {
    auto result = std::vector<std::string> {};
    const auto logger = [&](std::string&& s) { result.push_back(std::move(s)); };

    {
        const auto t [[maybe_unused]] = Timer {"", Timer::Unit::ms, 3, logger};  //
        EXPECT_FALSE(t.format().empty());
    }

    EXPECT_EQ(result.size(), 0);
}

TEST(Timer, Description) {
    auto result = std::vector<std::string> {};
    const auto logger = [&](std::string&& s) { result.push_back(std::move(s)); };

    {
        const auto t [[maybe_unused]] =
            Timer {"Description", Timer::Unit::ms, 3, logger};  //

        EXPECT_TRUE(t.format().starts_with("Description: 0"));
    }

    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result.at(0).starts_with("Description: 0"));
}

TEST(Timer, UnitStrings) {
    auto result = std::vector<std::string> {};
    const auto logger = [&](std::string&& s) { result.push_back(std::move(s)); };

    {
        const auto t [[maybe_unused]] = Timer {"", Timer::Unit::s, 0, logger};  //
        EXPECT_TRUE(t.format().ends_with("0s"));
    }
    {
        const auto t [[maybe_unused]] = Timer {"", Timer::Unit::ms, 0, logger};  //
        EXPECT_TRUE(t.format().ends_with("0ms"));
    }
    {
        const auto t [[maybe_unused]] = Timer {"", Timer::Unit::us, 0, logger};  //
        EXPECT_TRUE(t.format().ends_with("us"));
    }
    {
        const auto t [[maybe_unused]] = Timer {"", Timer::Unit::ns, 0, logger};  //
        EXPECT_TRUE(t.format().ends_with("ns"));
    }

    EXPECT_EQ(result.size(), 0);
}

TEST(Timer, CopyConstructor) {
    auto result = std::vector<std::string> {};
    const auto logger = [&](std::string&& s) { result.push_back(std::move(s)); };

    // copy construction
    {
        const auto t1 = Timer {"Test", Timer::Unit::ms, 3, logger};  //
        const auto t2 = Timer {t1};
        EXPECT_TRUE(t1.format().starts_with("Test: 0"));
        EXPECT_TRUE(t2.format().starts_with("Test: 0"));
    }

    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(result.at(0).starts_with("Test: 0"));
    EXPECT_TRUE(result.at(1).starts_with("Test: 0"));
}

TEST(Timer, CopyAssignment) {
    auto result1 = std::vector<std::string> {};
    const auto logger1 = [&](std::string&& s) { result1.push_back(std::move(s)); };

    auto result2 = std::vector<std::string> {};
    const auto logger2 = [&](std::string&& s) { result2.push_back(std::move(s)); };

    // copy assignment
    {
        const auto t1 = Timer {"Test", Timer::Unit::ms, 3, logger1};  //
        auto t2 = Timer {"Other", Timer::Unit::ms, 3, logger2};
        t2 = t1;

        EXPECT_TRUE(t1.format().starts_with("Test: 0"));
        EXPECT_TRUE(t2.format().starts_with("Test: 0"));

        EXPECT_EQ(result1.size(), 0);
        EXPECT_EQ(result2.size(), 1);
        EXPECT_TRUE(result2.at(0).starts_with("Other: 0"));
    }

    EXPECT_EQ(result1.size(), 2);
    EXPECT_TRUE(result1.at(0).starts_with("Test: 0"));
    EXPECT_TRUE(result1.at(1).starts_with("Test: 0"));

    EXPECT_EQ(result2.size(), 1);
    EXPECT_TRUE(result2.at(0).starts_with("Other: 0"));
}

TEST(Timer, MoveConstructor) {
    auto result = std::vector<std::string> {};
    const auto logger = [&](std::string&& s) { result.push_back(std::move(s)); };

    // copy construction
    {
        auto t1 = Timer {"Test", Timer::Unit::ms, 3, logger};  //
        const auto t2 = Timer {std::move(t1)};
        EXPECT_TRUE(t2.format().starts_with("Test: 0"));

        EXPECT_EQ(result.size(), 0);
    }

    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result.at(0).starts_with("Test: 0"));
}

TEST(Timer, MoveAssignment) {
    auto result1 = std::vector<std::string> {};
    const auto logger1 = [&](std::string&& s) { result1.push_back(std::move(s)); };

    auto result2 = std::vector<std::string> {};
    const auto logger2 = [&](std::string&& s) { result2.push_back(std::move(s)); };

    // move assignment
    {
        auto t1 = Timer {"Test", Timer::Unit::ms, 3, logger1};  //
        auto t2 = Timer {"Other", Timer::Unit::ms, 3, logger2};
        t2 = std::move(t1);

        EXPECT_TRUE(t2.format().starts_with("Test: 0"));

        EXPECT_EQ(result1.size(), 0);
        EXPECT_EQ(result2.size(), 1);
        EXPECT_TRUE(result2.at(0).starts_with("Other: 0"));
    }

    EXPECT_EQ(result1.size(), 1);
    EXPECT_TRUE(result1.at(0).starts_with("Test: 0"));

    EXPECT_EQ(result2.size(), 1);
    EXPECT_TRUE(result2.at(0).starts_with("Other: 0"));
}

// SelfCopyAssignment: not allowed, because of compiler warning
// SelfMoveAssignment: not allowed, because of compiler warning

}  // namespace logicsim
