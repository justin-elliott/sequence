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

#include <algorithm>
#include <iterator>

namespace jell {

namespace sequence_traits = detail::sequence_traits;

template <typename T, sequence_traits::traits auto Traits = sequence_traits::traits_t{}>
class sequence : private detail::storage<T, Traits>
{
public:
    using traits_type            = decltype(Traits);
    using value_type             = T;
    using size_type              = traits_type::size_type;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    sequence() = default;

    ~sequence() requires std::is_trivially_destructible_v<value_type> = default;
    ~sequence() { std::ranges::destroy(begin(), end()); }

    static constexpr const traits_type& traits() noexcept { return traits_; }

    constexpr size_type       capacity() const { return storage_.capacity(); }
    constexpr size_type       size()     const { return storage_.last() - storage_.first(); }
    constexpr pointer         data()           { return data_begin(); }
    constexpr const_pointer   data()     const { return data_begin(); }

    constexpr iterator        begin()          { return data_begin(); }
    constexpr const_iterator  begin()    const { return data_begin(); }
    constexpr const_iterator  cbegin()   const { return data_begin(); }
    constexpr iterator        end()            { return data_end(); }
    constexpr const_iterator  end()      const { return data_end(); }
    constexpr const_iterator  cend()     const { return data_end(); }

    constexpr reference       front()          { return *data_begin(); }
    constexpr const_reference front()    const { return *data_begin(); }
    constexpr reference       back()           { return *(std::prev(data_end())); }
    constexpr const_reference back()     const { return *(std::prev(data_end())); }

    template <typename... Args>
    constexpr reference unchecked_emplace_front(Args&&... args)
    {
        if constexpr (sequence_traits::is_front<Traits>) {
            move_right(1);
            ::new(data_begin()) value_type(std::forward<Args>(args)...);
            storage_.last(storage_.last() + 1);
        } else if constexpr (sequence_traits::is_middle<Traits>) {
            if (storage_.first() == 0) {
                const auto first = (capacity() - size() + 1) / 2;
                move_right(first);
                storage_.last(first + size());
                storage_.first(first);
            }
            ::new(std::prev(data_begin())) value_type(std::forward<Args>(args)...);
            storage_.first(storage_.first() - 1);
        } else if constexpr (sequence_traits::is_back<Traits>) {
            ::new(std::prev(data_begin())) value_type(std::forward<Args>(args)...);
            storage_.first(storage_.first() - 1);
        }
        return front();
    }

    template <typename... Args>
    constexpr reference unchecked_emplace_back(Args&&... args)
    {
        if constexpr (sequence_traits::is_front<Traits>) {
            ::new(data_end()) value_type(std::forward<Args>(args)...);
            storage_.last(storage_.last() + 1);
        } else if constexpr (sequence_traits::is_middle<Traits>) {
            if (storage_.last() == capacity()) {
                const auto first = (capacity() - size()) / 2;
                move_left(storage_.first() - first);
                storage_.last(first + size());
                storage_.first(first);
            }
            ::new(data_end()) value_type(std::forward<Args>(args)...);
            storage_.last(storage_.last() + 1);
        } else if constexpr (sequence_traits::is_back<Traits>) {
            move_left(1);
            ::new(std::prev(data_end())) value_type(std::forward<Args>(args)...);
            storage_.first(storage_.first() - 1);
        }
        return back();
    }

private:
    constexpr pointer       data_begin()       { return storage_.data(storage_.first()); }
    constexpr const_pointer data_begin() const { return storage_.data(storage_.first()); }
    constexpr pointer       data_end()         { return storage_.data(storage_.last()); }
    constexpr const_pointer data_end()   const { return storage_.data(storage_.last()); }

    /// Move [first..last) n elements left.
    constexpr void move_left(size_type n)
    {
        const auto n_uninitialized{std::min(n, size())};
        for (size_type i = 0; i != n_uninitialized; ++i) {
            ::new(std::prev(data_begin(), n - i)) value_type(std::move(*std::next(data_begin(), i)));
        }
        std::move(std::next(data_begin(), n_uninitialized), data_end(), data_begin());
        for (size_type i = n_uninitialized; i != 0; --i) {
            std::prev(data_end(), i)->~value_type();
        }
    }

    /// Move [first..last) n elements right.
    constexpr void move_right(size_type n)
    {
        const auto n_uninitialized{std::min(n, size())};
        for (size_type i = 0; i != n_uninitialized; ++i) {
            ::new(std::next(data_end(), n - i - 1)) value_type(std::move(*std::prev(data_end(), i + 1)));
        }
        std::move_backward(data_begin(), std::prev(data_end(), n_uninitialized), data_end());
        for (size_type i = 0; i != n_uninitialized; ++i) {
            std::next(data_begin(), i)->~value_type();
        }
    }

    static constexpr inline traits_type traits_{Traits};

    [[no_unique_address]] detail::storage<T, Traits> storage_;
};

} // namespace jell
