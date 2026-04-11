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

#include <type_traits>

namespace jell::detail {

/// An uninitialized value.
/// @tparam T The value type.
template <typename T>
union uninitialized
{
    constexpr uninitialized() noexcept requires std::is_trivially_constructible_v<T> = default;
    constexpr uninitialized() noexcept {}

    constexpr uninitialized(const uninitialized&) noexcept requires std::is_trivially_copy_constructible_v<T> = default;
    constexpr uninitialized(const uninitialized&) noexcept {}

    constexpr uninitialized(uninitialized&&) noexcept requires std::is_trivially_move_constructible_v<T> = default;
    constexpr uninitialized(uninitialized&&) noexcept {}

    constexpr ~uninitialized() requires std::is_trivially_destructible_v<T> = default;
    constexpr ~uninitialized() {}

    constexpr uninitialized& operator=(const uninitialized&) noexcept
        requires std::is_trivially_copy_assignable_v<T> = default;
    constexpr uninitialized& operator=(const uninitialized&) noexcept { return *this; }

    constexpr uninitialized& operator=(uninitialized&&) noexcept
        requires std::is_trivially_move_assignable_v<T> = default;
    constexpr uninitialized& operator=(uninitialized&&) noexcept { return *this; }

    T value;
};

} // namespace jell::detail
