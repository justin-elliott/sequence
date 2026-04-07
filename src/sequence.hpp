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

#include "detail/detached_range.hpp"
#include "detail/exception_guard.hpp"
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
        if constexpr (sequence_traits::is_middle<Traits>) {
            if (storage_.first() == 0) {
                detached_range detached{std::next(data_end(), (capacity() - size() + 1) / 2)};
                while (data_begin() != data_end()) {
                    detached.move_front(std::move(back()));
                    pop_back();
                }
                detached.attach(storage_);
            }
        }
        if constexpr (sequence_traits::is_middle<Traits> || sequence_traits::is_back<Traits>) {
            ::new(std::prev(data_begin())) value_type(std::forward<Args>(args)...);
            storage_.first(storage_.first() - 1);
        } else {
            detached_range detached{std::next(data_end())};
            while (data_begin() != data_end()) {
                detached.move_front(std::move(back()));
                pop_back();
            }
            ::new(data_begin()) value_type(std::forward<Args>(args)...);
            detached.attach_last(storage_);
        }
        return front();
    }

    template <typename... Args>
    constexpr reference unchecked_emplace_back(Args&&... args)
    {
        if constexpr (sequence_traits::is_middle<Traits>) {
            if (storage_.last() == capacity()) {
                detached_range detached{std::prev(data_begin(), (capacity() - size()) / 2)};
                while (data_begin() != data_end()) {
                    detached.move_back(std::move(front()));
                    pop_front();
                }
                detached.attach(storage_);
            }
        }
        if constexpr (sequence_traits::is_front<Traits> || sequence_traits::is_middle<Traits>) {
            ::new(data_end()) value_type(std::forward<Args>(args)...);
            storage_.last(storage_.last() + 1);
        } else if constexpr (sequence_traits::is_back<Traits>) {
            detached_range detached{std::prev(data_begin())};
            while (data_begin() != data_end()) {
                detached.move_back(std::move(front()));
                pop_front();
            }
            ::new(std::prev(data_end())) value_type(std::forward<Args>(args)...);
            detached.attach_first(storage_);
        }
        return back();
    }

    void pop_front()
    {
        if constexpr (sequence_traits::is_front<Traits>) {
            std::move(std::next(data_begin()), data_end(), data_begin());
            std::prev(data_end())->~value_type();
            storage_.last(storage_.last() - 1);
        } else {
            data_begin()->~value_type();
            storage_.first(storage_.first() + 1);
        }
    }

    void pop_back()
    {
        if constexpr (sequence_traits::is_back<Traits>) {
            std::move_backward(data_begin(), std::prev(data_end()), data_end());
            data_begin()->~value_type();
            storage_.first(storage_.first() + 1);
        } else {
            std::prev(data_end())->~value_type();
            storage_.last(storage_.last() - 1);
        }
    }

private:
    using detached_range = detail::detached_range<T, Traits>;
    using storage_type = detail::storage<T, Traits>;

    constexpr pointer       data_begin()       { return storage_.data(storage_.first()); }
    constexpr const_pointer data_begin() const { return storage_.data(storage_.first()); }
    constexpr pointer       data_end()         { return storage_.data(storage_.last()); }
    constexpr const_pointer data_end()   const { return storage_.data(storage_.last()); }

    static constexpr inline traits_type traits_{Traits};

    [[no_unique_address]] storage_type storage_;
};

} // namespace jell
