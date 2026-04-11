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
#include <stdexcept>
#include <type_traits>

namespace jell::detail::sequence_traits {

/// True if storage can be dynamically allocated.
struct dynamic
{
    bool value{true};
};

/// True if capacity can grow.
struct variable
{
    bool value{true};
};

/// The size of the fixed capacity (or the SBO).
template <std::unsigned_integral Size>
struct capacity
{
    using size_type = Size;
    size_type value{0};
};

template <std::integral Size>
capacity(Size) -> capacity<std::make_unsigned_t<Size>>;

template <typename>
struct is_capacity : std::false_type {};

template <std::unsigned_integral Size>
struct is_capacity<capacity<Size>> : std::true_type {};

template <std::integral Size>
struct is_capacity<Size> : std::true_type {};

template <typename T>
constexpr bool is_capacity_v = is_capacity<T>::value;

/// Layout of elements in a sequence.
enum class location : unsigned char
{
    front,  ///< Store elements from front-to-back (vector).
    middle, ///< Store elements from the middle-out (deque).
    back,   ///< Store elements from back-to-front (stack).
};

/// Growth rate of a sequence.
enum class growth : unsigned char
{
    exponential,            ///< Exponential growth.
    linear,                 ///< Linear growth.
    vector = exponential,   ///< Vector-equivalent growth.
};

/// The linear growth in elements.
struct increment
{
    std::size_t value{0};
};

/// The exponential growth factor.
struct factor
{
    float value{1.5f};
};

template <typename T>
concept specifier_t =
       std::is_same_v<dynamic, T>
    || std::is_same_v<variable, T>
    || is_capacity_v<T>
    || std::is_same_v<location, T>
    || std::is_same_v<growth, T>
    || std::is_same_v<increment, T>
    || std::is_same_v<factor, T>;

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

    bool        dynamic   = true;                //< True if storage can be dynamically allocated.
    bool        variable  = true;                //< True if capacity can grow.
    size_type   capacity  = 0;                   //< The size of the fixed capacity (or the SBO).
    location    location  = location::front;     //< The layout of elements.
    growth      growth    = growth::exponential; //< The sequence growth rate, combined with increment or factor.
    std::size_t increment = 0;                   //< The linear growth in elements (> 0).
    float       factor    = 1.5f;                //< The exponential growth factor (> 1.0).
};

/// Subset of traits defining an inplace sequence.
/// @tparam Size The type of the sequence's size member.
template <std::unsigned_integral Size = std::size_t>
struct inplace_t
{
    using size_type = std::type_identity_t<Size>;  //< The type of the size member.

    static const bool   dynamic  = false;           //< Inplace storage.
    size_type           capacity = 0;               //< The size of the fixed capacity.
    location            location = location::front; //< The layout of elements.
};

/// Check whether the traits define a variable sequence.
template <traits auto traits>
struct is_variable_type : std::bool_constant<traits.dynamic && traits.variable> {};

template <inplace_traits auto traits>
struct is_variable_type<traits> : std::false_type {};

/// Check whether the traits define a variable sequence.
template <traits auto traits>
constexpr bool is_variable = is_variable_type<traits>::value;

/// Check whether the traits define a front-to-back sequence.
template <traits auto traits>
constexpr bool is_front = (traits.location == location::front);

/// Check whether the traits define a middle-out sequence.
template <traits auto traits>
constexpr bool is_middle = (traits.location == location::middle);

/// Check whether the traits define a back-to-front sequence.
template <traits auto traits>
constexpr bool is_back = (traits.location == location::back);

struct default_capacity
{
    using size_type = std::size_t;
    static const inline std::size_t value = 0;
};

constexpr default_capacity get_capacity() { return default_capacity{}; }

template <specifier_t Head, specifier_t... Tail>
constexpr auto get_capacity(Head spec, Tail... tail)
{
    if constexpr (std::is_integral_v<Head>) {
        if (spec < 0) {
            throw std::invalid_argument("Capacity must not be negative");
        }
        using size_type = std::make_unsigned_t<Head>;
        return capacity<size_type>{static_cast<size_type>(spec)};
    } else if constexpr (is_capacity_v<Head>) {
        return spec;
    } else {
        return get_capacity(tail...);
    }
}

template <specifier_t Spec, specifier_t... Specifiers>
constexpr bool has_specifier = [] {
    if constexpr (sizeof...(Specifiers) == 0) {
        return false;
    } else {
        return [](specifier_t auto head, specifier_t auto... tail) {
            return std::is_same_v<Spec, decltype(head)> || has_specifier<Spec, decltype(tail)...>;
        }(Specifiers{}...);
    }
}();

template <specifier_t Spec>
constexpr Spec get_or_default(const Spec& default_value) { return default_value; }

template <specifier_t Spec, specifier_t Head, specifier_t... Tail>
constexpr Spec get_or_default(const Spec& default_value, Head spec, Tail... tail)
{
    if constexpr (std::is_same_v<Head, Spec>) {
        return spec;
    } else {
        return get_or_default(default_value, tail...);
    }
}

template <specifier_t... Specifiers>
constexpr bool all_unique = [] {
    if constexpr (sizeof...(Specifiers) <= 1) {
        return true;
    } else {
        return [](specifier_t auto head, specifier_t auto... tail) {
            return (!std::is_same_v<decltype(head), decltype(tail)> && ...)
                && (!is_capacity_v<decltype(head)> || (!is_capacity_v<decltype(tail)> && ...))
                && all_unique<decltype(tail)...>;
        }(Specifiers{}...);
    }
}();

template <specifier_t... Specifiers>
consteval auto make_traits(Specifiers... specs)
{
    static_assert(all_unique<Specifiers...>, "Specifiers must be unique");
    const auto cap = get_capacity(specs...);
    const bool is_cap_defaulted = std::is_same_v<std::remove_cv_t<decltype(cap)>, default_capacity>;
    const auto default_growth = (has_specifier<increment, Specifiers...> && !has_specifier<factor, Specifiers...>)
        ? growth::linear : growth::exponential;
    return traits_t<typename decltype(cap)::size_type>{
        .dynamic   = get_or_default(dynamic{is_cap_defaulted}, specs...).value,
        .variable  = get_or_default(variable{is_cap_defaulted}, specs...).value,
        .capacity  = cap.value,
        .location  = get_or_default(location::front, specs...),
        .growth    = get_or_default(default_growth, specs...),
        .increment = get_or_default(increment{0}, specs...).value,
        .factor    = get_or_default(factor{1.5f}, specs...).value,
    };
}

} // namespace jell::detail::sequence_traits
