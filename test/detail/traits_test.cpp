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

#include "detail/traits.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace jell;
using namespace jell::detail::sequence_traits;

// template <specifier_t... Specifiers>
// constexpr auto make_traits(Specifiers... specs)
// {
//     const auto cap = get_capacity(specs...);
//     const bool is_cap_defaulted = std::is_same_v<decltype(cap), default_capacity>;
//     return traits_t<typename decltype(cap)::size_type>{
//         .dynamic   = get_or_default(dynamic{is_cap_defaulted}, specs...).value,
//         .variable  = get_or_default(variable{is_cap_defaulted}, specs...).value,
//         .capacity  = cap.value,
//         .location  = get_or_default(location::front, specs...),
//         .growth    = get_or_default(growth::exponential, specs...),
//         .increment = get_or_default(increment{0}, specs...).value,
//         .factor    = get_or_default(factor{1.5f}, specs...).value,
//     };
// }

TEST(TraitsTest, make_traits_dynamic)
{
    EXPECT_TRUE(make_traits().dynamic);
    EXPECT_TRUE(make_traits(dynamic{}).dynamic);
    EXPECT_TRUE(make_traits(dynamic{true}).dynamic);
    EXPECT_FALSE(make_traits(dynamic{false}).dynamic);
    EXPECT_TRUE(make_traits(100, dynamic{true}).dynamic);
    EXPECT_FALSE(make_traits(100, dynamic{false}).dynamic);
    EXPECT_FALSE(make_traits(100).dynamic);
}

TEST(TraitsTest, make_traits_variable)
{
    EXPECT_TRUE(make_traits().variable);
    EXPECT_TRUE(make_traits(variable{}).variable);
    EXPECT_TRUE(make_traits(variable{true}).variable);
    EXPECT_FALSE(make_traits(variable{false}).variable);
    EXPECT_TRUE(make_traits(100, variable{true}).variable);
    EXPECT_FALSE(make_traits(100, variable{false}).variable);
    EXPECT_FALSE(make_traits(100).variable);
}

TEST(TraitsTest, make_traits_capacity)
{
    EXPECT_EQ(make_traits().capacity, 0u);
    EXPECT_EQ(make_traits(100).capacity, 100u);
    EXPECT_EQ(make_traits(capacity{100}).capacity, 100u);
    EXPECT_EQ(make_traits(capacity<std::uint8_t>{100}).capacity, 100u);
    EXPECT_EQ(make_traits(dynamic{}, variable{}, 100).capacity, 100u);
}

TEST(TraitsTest, make_traits_location)
{
    EXPECT_EQ(make_traits().location, location::front);
    EXPECT_EQ(make_traits(location::front).location, location::front);
    EXPECT_EQ(make_traits(location::middle).location, location::middle);
    EXPECT_EQ(make_traits(location::back).location, location::back);
    EXPECT_EQ(make_traits(100, location::front).location, location::front);
    EXPECT_EQ(make_traits(100, location::middle).location, location::middle);
    EXPECT_EQ(make_traits(100, location::back).location, location::back);
}

TEST(TraitsTest, make_traits_growth)
{
    EXPECT_EQ(make_traits().growth, growth::exponential);
    EXPECT_EQ(make_traits(growth::linear).growth, growth::linear);
    EXPECT_EQ(make_traits(growth::exponential).growth, growth::exponential);
    EXPECT_EQ(make_traits(increment{100}).growth, growth::linear);
    EXPECT_EQ(make_traits(increment{100}, factor{2.0f}).growth, growth::exponential);
    EXPECT_EQ(make_traits(100, growth::linear).growth, growth::linear);
    EXPECT_EQ(make_traits(100, growth::exponential).growth, growth::exponential);
    EXPECT_EQ(make_traits(100, increment{100}).growth, growth::linear);
    EXPECT_EQ(make_traits(100, increment{100}, factor{2.0f}).growth, growth::exponential);
}

TEST(TraitsTest, make_traits_increment)
{
    EXPECT_EQ(make_traits().increment, 0);
    EXPECT_EQ(make_traits(increment{100}).increment, 100);
    EXPECT_EQ(make_traits(100, increment{100}).increment, 100);
}

TEST(TraitsTest, make_traits_factor)
{
    EXPECT_EQ(make_traits().factor, 1.5f);
    EXPECT_EQ(make_traits(factor{2.5f}).factor, 2.5f);
    EXPECT_EQ(make_traits(100, factor{2.5f}).factor, 2.5f);
}
