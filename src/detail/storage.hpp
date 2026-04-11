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

/// @brief Storage for a sequence.
/// @tparam T The type of elements stored in the sequence.
/// @tparam traits Traits defining the sequence.
template <typename T, sequence_traits::traits auto Traits>
class storage;

/// Storage for a zero-length inplace sequence.
template <typename T, sequence_traits::traits auto Traits>
    requires (!Traits.dynamic) && (Traits.capacity == 0)
class storage<T, Traits>
{
public:
    using size_type = typename decltype(Traits)::size_type;

    static constexpr size_type capacity()       noexcept { return 0; }
    static constexpr T*        data(size_type)  noexcept { return nullptr; }
    static constexpr size_type first()          noexcept { return 0; }
    static constexpr void      first(size_type) noexcept {}
    static constexpr size_type last()           noexcept { return 0; }
    static constexpr void      last(size_type)  noexcept {}
    static constexpr void      reset()          noexcept {}
};

/// Storage for a front inplace sequence.
template <typename T, sequence_traits::traits auto Traits>
    requires (!Traits.dynamic) && (Traits.capacity != 0) && sequence_traits::is_front<Traits>
class storage<T, Traits>
{
public:
    using size_type = typename decltype(Traits)::size_type;

    static constexpr size_type capacity()              noexcept { return Traits.capacity; }
    constexpr        T*        data(size_type i)       noexcept { return std::addressof(data_[i].value); }
    constexpr        const T*  data(size_type i) const noexcept { return std::addressof(data_[i].value); }
    static constexpr size_type first()                 noexcept { return 0; }
    static constexpr void      first(size_type)        noexcept {}
    constexpr        size_type last()            const noexcept { return last_; }
    constexpr        void      last(size_type i)       noexcept { last_ = i; }
    constexpr        void      reset()                 noexcept { last_ = 0; }

private:
    size_type last_{0};
    uninitialized<T> data_[capacity()];
};

/// Storage for a middle inplace sequence.
template <typename T, sequence_traits::traits auto Traits>
    requires (!Traits.dynamic) && (Traits.capacity != 0) && sequence_traits::is_middle<Traits>
class storage<T, Traits>
{
public:
    using size_type = typename decltype(Traits)::size_type;

    static constexpr size_type capacity()              noexcept { return Traits.capacity; }
    constexpr        T*        data(size_type i)       noexcept { return std::addressof(data_[i].value); }
    constexpr        const T*  data(size_type i) const noexcept { return std::addressof(data_[i].value); }
    constexpr        size_type first()           const noexcept { return first_; }
    constexpr        void      first(size_type i)      noexcept { first_ = i; }
    constexpr        size_type last()            const noexcept { return last_; }
    constexpr        void      last(size_type i)       noexcept { last_ = i; }
    constexpr        void      reset()                 noexcept { first_ = last_ = capacity() / 2; }

private:
    size_type first_{capacity() / 2};
    size_type last_{capacity() / 2};
    uninitialized<T> data_[capacity()];
};

/// Storage for a back inplace sequence.
template <typename T, sequence_traits::traits auto Traits>
    requires (!Traits.dynamic) && (Traits.capacity != 0) && sequence_traits::is_back<Traits>
class storage<T, Traits>
{
public:
    using size_type = typename decltype(Traits)::size_type;

    static constexpr size_type capacity()              noexcept { return Traits.capacity; }
    constexpr        T*        data(size_type i)       noexcept { return std::addressof(data_[i].value); }
    constexpr        const T*  data(size_type i) const noexcept { return std::addressof(data_[i].value); }
    constexpr        size_type first()           const noexcept { return first_; }
    constexpr        void      first(size_type i)      noexcept { first_ = i; }
    static constexpr size_type last()                  noexcept { return capacity(); }
    static constexpr void      last(size_type)         noexcept {}
    constexpr        void      reset()                 noexcept { first_ = capacity(); }

private:
    size_type first_{capacity()};
    uninitialized<T> data_[capacity()];
};

} // namespace jell::detail
