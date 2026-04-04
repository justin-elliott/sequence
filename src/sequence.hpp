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

namespace jell {

namespace sequence_traits = detail::sequence_traits;

template <typename T, sequence_traits::traits auto traits = sequence_traits::traits_t{}>
class sequence
{
public:
    using size_type = decltype(traits)::size_type;

    constexpr size_type capacity() const noexcept { return storage_.capacity(); }
    constexpr size_type size()     const noexcept { return storage_.size(); }

private:
    static consteval bool capacity_bounded() noexcept
    {
        return (traits.capacity <= std::numeric_limits<size_type>::max());
    }

    static_assert(capacity_bounded(), "Capacity exceeds the bounds of size_type");

    [[no_unique_address]] detail::storage<T, traits> storage_;
};

} // namespace jell
