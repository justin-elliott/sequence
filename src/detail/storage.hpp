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

#include "detail/traits.hpp"
#include "detail/uninitialized.hpp"

#include <memory>

namespace jell::detail {

template <typename T, sequence_traits::traits auto traits>
class storage;

template <typename T, sequence_traits::traits auto traits>
    requires (!traits.dynamic) && (traits.capacity == 0)
class storage<T, traits>
{
public:
    using size_type = typename decltype(traits)::size_type;

    static constexpr size_type capacity()            noexcept { return 0; }
    static constexpr void      capacity(size_type)   noexcept {}
    static constexpr size_type size()                noexcept { return 0; }
    static constexpr void      size(size_type)       noexcept {}
    static constexpr T*        data()                noexcept { return nullptr; }
};

template <typename T, sequence_traits::traits auto traits>
    requires (!traits.dynamic) && (traits.capacity != 0)
class storage<T, traits>
{
public:
    using size_type = typename decltype(traits)::size_type;

    static constexpr size_type capacity()            noexcept { return static_cast<size_type>(traits.capacity); }
    static constexpr void      capacity(size_type)   noexcept {}
    constexpr size_type        size()          const noexcept { return size_; }
    constexpr void             size(size_type n)     noexcept { size_ = n; }
    constexpr T*               data()                noexcept { return std::addressof(data_[0].value); }
    constexpr const T*         data()          const noexcept { return std::addressof(data_[0].value); }

private:
    size_type size_{0};
    uninitialized<T> data_[capacity()];
};

} // namespace jell::detail
