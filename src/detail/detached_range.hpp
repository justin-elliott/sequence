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

#include "detail/storage.hpp"
#include "detail/traits.hpp"

#include <ranges>

namespace jell::detail {

template <typename T, sequence_traits::traits auto Traits>
class detached_range
{
public:
    using value_type   = T;
    using pointer      = value_type*;
    using storage_type = storage<T, Traits>;
    using size_type    = storage_type::size_type;

    explicit detached_range(pointer pos)
        : begin_{pos}
        , end_{pos}
    {}

    ~detached_range() { destroy(); }

    void attach(storage_type& storage)
    {
        storage.first(static_cast<size_type>(std::distance(storage.data(0), begin_)));
        storage.last(static_cast<size_type>(std::distance(storage.data(0), end_)));
        begin_ = end_;
    }

    void attach_first(storage_type& storage)
    {
        storage.first(static_cast<size_type>(std::distance(storage.data(0), begin_)));
        begin_ = end_;
    }

    void attach_last(storage_type& storage)
    {
        storage.last(static_cast<size_type>(std::distance(storage.data(0), end_)));
        begin_ = end_;
    }

    void move_front(value_type&& value)
    {
        ::new(std::prev(begin_)) value_type(std::move(value));
        --begin_;
    }

    void move_back(value_type&& value)
    {
        ::new(end_) value_type(std::move(value));
        ++end_;
    }

private:
    void destroy() { std::ranges::destroy(begin_, end_); }

    pointer begin_;
    pointer end_;
};

} // namespace jell::detail
