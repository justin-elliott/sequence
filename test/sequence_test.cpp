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
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace jell;
using namespace jell::sequence_traits;

const std::uint8_t inplace_size = 32;

template <typename T>
class SequenceTest : public testing::Test
{
protected:
    using value_type = T::value_type;
    using size_type = T::size_type;

    static constexpr std::size_t value_count() noexcept
    {
        return (is_variable<T::traits()>) ? inplace_size : T::max_size();
    }

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
        return std::views::iota(size_type{100}, static_cast<size_type>(100 + value_count()))
             | std::views::transform([](size_type i) { return make_value_type(i); });
    }

    struct reversed_if_front_t : public std::ranges::range_adaptor_closure<reversed_if_front_t>
    {
        constexpr auto operator()(auto&& rg) const
        {
            if constexpr (is_back<T::traits()>) {
                return rg | std::views::reverse;
            } else {
                return rg;
            }
        }
    };

    constexpr reversed_if_front_t reversed_if_front() { return reversed_if_front_t{}; }
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

    static inline const std::size_t error_value = static_cast<std::size_t>(-1);
    std::size_t value = error_value;
};

struct NonTrivial
{
    constexpr NonTrivial(std::size_t value = 0) : value{value} {}

    constexpr NonTrivial(const NonTrivial& other) { value = other.value; }
    constexpr NonTrivial(NonTrivial&& other) { value = other.value; }

    constexpr NonTrivial& operator=(const NonTrivial& other) { value = other.value; return *this; }
    constexpr NonTrivial& operator=(NonTrivial&& other) { value = other.value; return *this; }

    constexpr ~NonTrivial() { value = error_value; }

    constexpr friend bool operator==(const NonTrivial&, const NonTrivial&) = default;

    static inline const std::size_t error_value = static_cast<std::size_t>(-1);
    std::size_t value = error_value;
};

template <typename T>
concept error_value_type = requires(T obj) {
    T::error_value;
    obj.value;
};

template <error_value_type T>
struct std::formatter<T>
{
    constexpr auto parse(auto&& ctx) { return ctx.begin(); }

    auto format(const T& obj, auto&& ctx) const
    {
        if (obj.value == T::error_value) {
            return std::format_to(ctx.out(), "(/)");
        }
        return std::format_to(ctx.out(), "{}", obj.value);
    }
};

using sequence_types = testing::Types<
    sequence<double, 0>,
    sequence<std::uint16_t, inplace_size>,
    sequence<std::uint16_t, inplace_size, location::middle>,
    sequence<std::uint16_t, inplace_size, location::back>,
    sequence<MoveOnly, inplace_size>,
    sequence<MoveOnly, inplace_size, location::middle>,
    sequence<MoveOnly, inplace_size, location::back>,
    sequence<NonTrivial, inplace_size>,
    sequence<NonTrivial, inplace_size, location::middle>,
    sequence<NonTrivial, inplace_size, location::back>
>;
TYPED_TEST_SUITE(SequenceTest, sequence_types);

TYPED_TEST(SequenceTest, default_construct)
{
    TypeParam seq;
    EXPECT_TRUE(seq.empty()) << std::format("{}", seq);
    EXPECT_EQ(seq.size(), 0) << std::format("{}", seq);
}

TYPED_TEST(SequenceTest, construct_count)
{
    if constexpr (!std::is_default_constructible_v<typename TypeParam::value_type>) {
        GTEST_SKIP() << "Not default constructible";
    } else {
        TypeParam seq(this->value_count());
        EXPECT_EQ(seq.size(), this->value_count());
    }
}

TYPED_TEST(SequenceTest, construct_count_value)
{
    if constexpr (!std::is_copy_constructible_v<typename TypeParam::value_type>) {
        GTEST_SKIP() << "Not copy constructible";
    } else {
        TypeParam seq(this->value_count(), this->make_value_type(100));
        EXPECT_EQ(seq.size(), this->value_count());
    }
}

TYPED_TEST(SequenceTest, construct_iterators)
{
    auto values = this->values() | std::ranges::to<std::vector>();
    TypeParam seq(std::make_move_iterator(std::ranges::begin(values)),
                  std::make_move_iterator(std::ranges::end(values)));
    EXPECT_TRUE(std::ranges::equal(seq, this->values() | this->reversed_if_front()))
        << std::format("{} != {}", seq, this->values() | this->reversed_if_front());
}

TYPED_TEST(SequenceTest, construct_range)
{
    TypeParam seq(std::from_range, this->values() | std::views::as_rvalue);
    EXPECT_TRUE(std::ranges::equal(seq, this->values() | this->reversed_if_front()))
        << std::format("{} != {}", seq, this->values() | this->reversed_if_front());
}

TYPED_TEST(SequenceTest, construct_copy)
{
    if constexpr (!std::is_copy_constructible_v<typename TypeParam::value_type>) {
        GTEST_SKIP() << "Not copy constructible";
    } else {
        const TypeParam seq1(std::from_range, this->values());
        const TypeParam seq2(seq1);
        EXPECT_TRUE(std::ranges::equal(seq1, seq2))
            << std::format("{} != {}", seq1, seq2);
    }
}

TYPED_TEST(SequenceTest, construct_move)
{
    if constexpr (!std::is_move_constructible_v<typename TypeParam::value_type>) {
        GTEST_SKIP() << "Not move constructible";
    } else {
        TypeParam seq1(std::from_range, this->values());
        const TypeParam seq2(std::move(seq1));
        const TypeParam expected(std::from_range, this->values());
        EXPECT_TRUE(std::ranges::equal(seq2, expected))
            << std::format("{} != {}", seq2, expected);
    }
}

TYPED_TEST(SequenceTest, construct_initializer_list)
{
    if constexpr (!std::is_copy_assignable_v<typename TypeParam::value_type>) {
        GTEST_SKIP() << "Not copy assignable";
    } else if constexpr (TypeParam::max_size() < 5) {
        GTEST_SKIP() << "Capacity too small";
    } else {
        const auto values = this->values() | std::views::take(5) | std::ranges::to<std::vector>();
        TypeParam seq{values[0], values[1], values[2], values[3], values[4]};
        EXPECT_TRUE(std::ranges::equal(seq, values | this->reversed_if_front()))
            << std::format("{} != {}", seq, values | this->reversed_if_front());
    }
}

TYPED_TEST(SequenceTest, assign_copy)
{
    if constexpr (!std::is_copy_assignable_v<typename TypeParam::value_type>) {
        GTEST_SKIP() << "Not copy assignable";
    } else {
        const TypeParam seq1(std::from_range, this->values());
        TypeParam seq2;
        seq2 = seq1;
        EXPECT_TRUE(std::ranges::equal(seq1, seq2))
            << std::format("{} != {}", seq1, seq2);
    }
}

TYPED_TEST(SequenceTest, assign_move)
{
    if constexpr (!std::is_move_assignable_v<typename TypeParam::value_type>) {
        GTEST_SKIP() << "Not move assignable";
    } else {
        TypeParam seq1(std::from_range, this->values());
        TypeParam seq2;
        seq2 = std::move(seq1);
        const TypeParam expected(std::from_range, this->values());
        EXPECT_TRUE(std::ranges::equal(seq2, expected))
            << std::format("{} != {}", seq2, expected);
    }
}

TYPED_TEST(SequenceTest, unchecked_emplace_front)
{
    TypeParam seq;

    const auto expected_emplaced = this->values();
    const auto expected_values = expected_emplaced | std::views::reverse | std::ranges::to<std::vector>();
    auto moveable_values = this->values();

    for (auto [moveable_value, expected_value] : std::views::zip(moveable_values, expected_emplaced)) {
        EXPECT_EQ(seq.unchecked_emplace_front(std::move(moveable_value)), expected_value);
    }
    EXPECT_TRUE(std::ranges::equal(seq, expected_values))
        << std::format("{} != {}", seq, expected_values);
}

TYPED_TEST(SequenceTest, try_emplace_front)
{
    TypeParam seq;
    EXPECT_EQ((seq.try_emplace_front(this->make_value_type(100)) != nullptr), (seq.capacity() > 0));
}

TYPED_TEST(SequenceTest, try_emplace_front_at_capacity)
{
    TypeParam seq;
    if constexpr (is_variable<seq.traits()>) {
        GTEST_SKIP() << "Variable capacity";
    } else {
        while (seq.size() < seq.capacity()) {
            EXPECT_NE(seq.try_emplace_front(this->make_value_type(0)), nullptr);
        }
        EXPECT_EQ(seq.try_emplace_front(this->make_value_type(100)), nullptr);
    }
}

TYPED_TEST(SequenceTest, emplace_front)
{
    TypeParam seq;
    if (seq.max_size() > 0) {
        EXPECT_EQ(seq.emplace_front(this->make_value_type(100)), this->make_value_type(100));
    }
}

TYPED_TEST(SequenceTest, emplace_front_at_capacity)
{
    TypeParam seq;
    if constexpr (is_variable<seq.traits()>) {
        GTEST_SKIP() << "Variable capacity";
    } else {
        while (seq.size() < seq.capacity()) {
            const auto expected_value = this->make_value_type(seq.size());
            EXPECT_EQ(seq.emplace_front(this->make_value_type(seq.size())), expected_value);
        }
        EXPECT_THROW(seq.emplace_front(this->make_value_type(100)), std::bad_alloc);
    }
}

TYPED_TEST(SequenceTest, unchecked_emplace_back)
{
    TypeParam seq;

    const auto expected_values = this->values();
    auto moveable_values = this->values();

    for (auto [moveable_value, expected_value] : std::views::zip(moveable_values, expected_values)) {
        EXPECT_EQ(seq.unchecked_emplace_back(std::move(moveable_value)), expected_value);
    }
    EXPECT_TRUE(std::ranges::equal(seq, expected_values))
        << std::format("{} != {}", seq, expected_values);
}

TYPED_TEST(SequenceTest, try_emplace_back)
{
    TypeParam seq;
    EXPECT_EQ((seq.try_emplace_back(this->make_value_type(100)) != nullptr), (seq.capacity() > 0));
}

TYPED_TEST(SequenceTest, try_emplace_back_at_capacity)
{
    TypeParam seq;
    if constexpr (is_variable<seq.traits()>) {
        GTEST_SKIP() << "Variable capacity";
    } else {
        while (seq.size() < seq.capacity()) {
            EXPECT_NE(seq.try_emplace_back(this->make_value_type(0)), nullptr);
        }
        EXPECT_EQ(seq.try_emplace_back(this->make_value_type(100)), nullptr);
    }
}

TYPED_TEST(SequenceTest, emplace_back)
{
    TypeParam seq;
    if (seq.max_size() > 0) {
        EXPECT_EQ(seq.emplace_back(this->make_value_type(100)), this->make_value_type(100));
    }
}

TYPED_TEST(SequenceTest, emplace_back_at_capacity)
{
    TypeParam seq;
    if constexpr (is_variable<seq.traits()>) {
        GTEST_SKIP() << "Variable capacity";
    } else {
        while (seq.size() < seq.capacity()) {
            const auto expected_value = this->make_value_type(seq.size());
            EXPECT_EQ(seq.emplace_back(this->make_value_type(seq.size())), expected_value);
        }
        EXPECT_THROW(seq.emplace_back(this->make_value_type(100)), std::bad_alloc);
    }
}

TYPED_TEST(SequenceTest, unchecked_emplace_native)
{
    TypeParam seq;
    for (auto [moveable_value, expected_value] : std::views::zip(this->values(), this->values())) {
        EXPECT_EQ(seq.unchecked_emplace_native(std::move(moveable_value)), expected_value);
    }
    EXPECT_TRUE(std::ranges::equal(seq, this->values() | this->reversed_if_front()))
        << std::format("{} != {}", seq, this->values() | this->reversed_if_front());
}

TYPED_TEST(SequenceTest, try_emplace_native)
{
    TypeParam seq;
    EXPECT_EQ((seq.try_emplace_native(this->make_value_type(100)) != nullptr), (seq.capacity() > 0));
}

TYPED_TEST(SequenceTest, try_emplace_native_at_capacity)
{
    TypeParam seq;
    if constexpr (is_variable<seq.traits()>) {
        GTEST_SKIP() << "Variable capacity";
    } else {
        while (seq.size() < seq.capacity()) {
            EXPECT_NE(seq.try_emplace_native(this->make_value_type(0)), nullptr);
        }
        EXPECT_EQ(seq.try_emplace_native(this->make_value_type(100)), nullptr);
    }
}

TYPED_TEST(SequenceTest, emplace_native)
{
    TypeParam seq;
    if (seq.max_size() > 0) {
        EXPECT_EQ(seq.emplace_native(this->make_value_type(100)), this->make_value_type(100));
    }
}

TYPED_TEST(SequenceTest, emplace_native_at_capacity)
{
    TypeParam seq;
    if constexpr (is_variable<seq.traits()>) {
        GTEST_SKIP() << "Variable capacity";
    } else {
        while (seq.size() < seq.capacity()) {
            const auto expected_value = this->make_value_type(seq.size());
            EXPECT_EQ(seq.emplace_native(this->make_value_type(seq.size())), expected_value);
        }
        EXPECT_THROW(seq.emplace_native(this->make_value_type(100)), std::bad_alloc);
    }
}

TYPED_TEST(SequenceTest, clear)
{
    auto values = this->values() | std::views::as_rvalue;
    TypeParam seq(std::from_range, values);
    EXPECT_EQ(seq.size(), std::ranges::size(values));

    seq.clear();
    EXPECT_TRUE(seq.empty());
    EXPECT_EQ(seq.size(), 0);
}