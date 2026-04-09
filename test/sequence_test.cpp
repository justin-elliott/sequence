// MIT License
//
// Copyright (c) 2026 Justin Elliott (github.com/justin-elliott)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "sequence.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <ranges>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace jell;
using namespace jell::sequence_traits;

template <typename T>
class SequenceTest : public testing::Test
{
protected:
    using value_type = T::value_type;
    using size_type = T::size_type;

    static constexpr value_type make_value_type(size_type value)
    {
        if constexpr (std::is_arithmetic_v<value_type>) {
            return static_cast<value_type>(value);
        } else {
            return value_type{value};
        }
    }

    static constexpr auto values()
    {
        return std::views::iota(size_type{100})
             | std::views::transform([](size_type i) { return make_value_type(i); });
    }

    static constexpr auto values(size_type count)
    {
        return values() | std::views::take(count);
    }
};

struct MoveOnly
{
    constexpr MoveOnly() = delete;
    constexpr explicit MoveOnly(std::size_t value) : value{value} {}

    constexpr MoveOnly(const MoveOnly&) = delete;
    constexpr MoveOnly& operator=(const MoveOnly&) = delete;

    constexpr MoveOnly(MoveOnly&&) = default;
    constexpr MoveOnly& operator=(MoveOnly&&) = default;

    constexpr ~MoveOnly() { value = error_value; }

    constexpr friend bool operator==(const MoveOnly&, const MoveOnly&) = default;

    static inline const std::size_t error_value{static_cast<std::size_t>(-1)};
    std::size_t value{error_value};
};

template <>
struct std::formatter<MoveOnly>
{
    constexpr auto parse(auto&& ctx) { return ctx.begin(); }

    auto format(const MoveOnly& move_only, auto&& ctx) const
    {
        if (move_only.value == MoveOnly::error_value) {
            return std::format_to(ctx.out(), "(/)");
        }
        return std::format_to(ctx.out(), "{}", move_only.value);
    }
};

const std::size_t default_size{32};

using sequence_types = testing::Types<
    sequence<double, inplace_t{0}>,
    sequence<std::uint16_t, inplace_t{default_size, sequence_traits::location::front}>,
    sequence<std::uint16_t, inplace_t{default_size, sequence_traits::location::middle}>,
    sequence<std::uint16_t, inplace_t{default_size, sequence_traits::location::back}>,
    sequence<MoveOnly, inplace_t{default_size, sequence_traits::location::front}>,
    sequence<MoveOnly, inplace_t{default_size, sequence_traits::location::middle}>,
    sequence<MoveOnly, inplace_t{default_size, sequence_traits::location::back}>
>;
TYPED_TEST_SUITE(SequenceTest, sequence_types);

TYPED_TEST(SequenceTest, can_construct)
{
    TypeParam seq;
    EXPECT_TRUE(seq.empty()) << std::format("{}", seq);
    EXPECT_EQ(seq.size(), 0) << std::format("{}", seq);
}

TYPED_TEST(SequenceTest, can_unchecked_emplace_front)
{
    TypeParam seq;

    const auto n_values = is_variable<seq.traits()> ? default_size : seq.capacity();
    const auto expected_emplaced = this->values(n_values);
    auto moveable_values = this->values();

    for (auto [moveable_value, expected_value] : std::views::zip(moveable_values, expected_emplaced)) {
        EXPECT_EQ(seq.unchecked_emplace_front(std::move(moveable_value)), expected_value);
    }
    auto expected_values = expected_emplaced | std::views::reverse;
    EXPECT_TRUE(std::ranges::equal(seq, expected_values))
        << std::format("{} != {}", seq, expected_values);
}

TYPED_TEST(SequenceTest, can_unchecked_emplace_back)
{
    TypeParam seq;

    const auto n_values = is_variable<seq.traits()> ? default_size : seq.capacity();
    const auto expected_values = this->values(n_values);
    auto moveable_values = this->values();

    for (auto [moveable_value, expected_value] : std::views::zip(moveable_values, expected_values)) {
        EXPECT_EQ(seq.unchecked_emplace_back(std::move(moveable_value)), expected_value);
    }
    EXPECT_TRUE(std::ranges::equal(seq, expected_values))
        << std::format("{} != {}", seq, expected_values);
}
