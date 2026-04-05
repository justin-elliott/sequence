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

#pragma once

#include "detail/uninitialized.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace jell::detail::sequence_traits {

/// Layout of elements in a sequence.
enum class location : unsigned char
{
    front,  ///< Store elements from front-to-back (vector).
    middle, ///< Store elements from the middle out (deque).
    back,   ///< Store elements from back-to-front (stack).
};

/// Growth rate of a sequence.
enum class growth : unsigned char
{
    exponential,            ///< Exponential growth.
    linear,                 ///< Linear growth.
    vector = exponential,   ///< Vector-equivalent growth.
};

/// General traits defining a sequence.
template <typename Traits>
concept general_traits = requires(Traits traits)
{
    typename Traits::size_type;
    {traits.dynamic} -> std::convertible_to<bool>;
    {traits.variable} -> std::convertible_to<bool>;
    {traits.capacity} -> std::convertible_to<typename Traits::size_type>;
    {traits.location} -> std::convertible_to<location>;
    {traits.growth} -> std::convertible_to<growth>;
    {traits.increment} -> std::convertible_to<std::size_t>;
    {traits.factor} -> std::convertible_to<float>;
};

/// Subset of traits defining an inplace sequence.
template <typename Traits>
concept inplace_traits = requires(Traits traits)
{
    typename Traits::size_type;
    {traits.dynamic} -> std::convertible_to<bool>;
    {traits.capacity} -> std::convertible_to<typename Traits::size_type>;
    {traits.location} -> std::convertible_to<location>;
};

/// Traits defining a sequence.
template <typename Traits>
concept traits = general_traits<Traits> || inplace_traits<Traits>;

/// General traits defining a sequence.
/// @tparam Size The type of the sequence's size member.
template <std::unsigned_integral Size = std::size_t>
struct traits_t
{
    using size_type = std::type_identity_t<Size>; //< The type of the size member.

    bool        dynamic  {true};                //< True if storage can be dynamically allocated.
    bool        variable {true};                //< True if capacity can grow.
    size_type   capacity {0};                   //< The size of the fixed capacity (or the SBO).
    location    location {location::front};     //< The layout of elements.
    growth      growth   {growth::exponential}; //< The sequence growth rate, combined with increment or factor.
    std::size_t increment{0};                   //< The linear growth in elements (> 0).
    float       factor   {1.5f};                //< The exponential growth factor (> 1.0).
};

/// Subset of traits defining an inplace sequence.
/// @tparam Size The type of the sequence's size member.
template <std::unsigned_integral Size = std::size_t>
struct inplace_t
{
    using size_type = std::type_identity_t<Size>;  //< The type of the size member.

    static const bool   dynamic {false};           //< Inplace storage.
    size_type           capacity{0};               //< The size of the fixed capacity.
    location            location{location::front}; //< The layout of elements.
};

} // namespace jell::detail::sequence_traits
