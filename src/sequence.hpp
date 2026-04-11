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

#include "detail/container_compatible_range.hpp"
#include "detail/exception_guard.hpp"
#include "detail/range_guard.hpp"
#include "detail/storage.hpp"
#include "detail/traits.hpp"

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <limits>

namespace jell {

namespace sequence_traits = detail::sequence_traits;

template <typename T, auto... Traits>
class sequence;

template <typename T, sequence_traits::traits auto Traits>
class sequence<T, Traits>
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

    constexpr sequence() = default;

    constexpr explicit sequence(size_type count);
    constexpr sequence(size_type count, const value_type& value);

    template <std::input_iterator Iterator, std::input_iterator Sentinel>
    constexpr sequence(Iterator first, Sentinel last)
    {
        if constexpr (std::random_access_iterator<Iterator>) {
            ensure_can_grow_by(std::distance(first, last));
        }

        detail::exception_guard guard{&sequence::destroy_all, this};
        if constexpr (std::random_access_iterator<Iterator>) {
            for (; first != last; ++first) {
                unchecked_emplace_native(*first);
            }
        } else {
            for (; first != last; ++first) {
                emplace_native(*first);
            }
        }
        guard.release();
    }

    template <detail::container_compatible_range<value_type> Range>
    constexpr sequence(std::from_range_t, Range&& rg)
        : sequence(std::ranges::begin(rg), std::ranges::end(rg))
    {
    }

    constexpr sequence(const sequence& other);
    constexpr sequence(sequence&& other)
        noexcept(max_size() == 0 || std::is_nothrow_move_constructible_v<value_type>);
    
    constexpr sequence(std::initializer_list<value_type> init)
        : sequence(init.begin(), init.end())
    {}

    constexpr ~sequence() requires std::is_trivially_destructible_v<value_type> = default;
    constexpr ~sequence() { destroy_all(); }

    static constexpr const traits_type& traits() noexcept { return traits_; }

    [[nodiscard]] static constexpr size_type max_size()
    {
        if constexpr (sequence_traits::is_variable<Traits>) {
            const auto max_repr = static_cast<std::size_t>(
                std::numeric_limits<difference_type>::max() / sizeof(value_type));
            return (max_repr > std::numeric_limits<size_type>::max())
                ? std::numeric_limits<size_type>::max()
                : static_cast<size_type>(max_repr);
        } else {
            return Traits.capacity;
        }
    }

    [[nodiscard]] static constexpr size_type capacity()
        requires (!sequence_traits::is_variable<Traits>)
    {
        return Traits.capacity;
    }

    [[nodiscard]] constexpr size_type       capacity() const { return storage_.capacity(); }

    [[nodiscard]] constexpr bool            empty()    const { return storage_.last() == storage_.first(); }
    [[nodiscard]] constexpr size_type       size()     const { return storage_.last() - storage_.first(); }
    [[nodiscard]] constexpr pointer         data()           { return data_begin(); }
    [[nodiscard]] constexpr const_pointer   data()     const { return data_begin(); }

    [[nodiscard]] constexpr iterator        begin()          { return data_begin(); }
    [[nodiscard]] constexpr const_iterator  begin()    const { return data_begin(); }
    [[nodiscard]] constexpr const_iterator  cbegin()   const { return data_begin(); }
    [[nodiscard]] constexpr iterator        end()            { return data_end(); }
    [[nodiscard]] constexpr const_iterator  end()      const { return data_end(); }
    [[nodiscard]] constexpr const_iterator  cend()     const { return data_end(); }

    [[nodiscard]] constexpr reference       front()          { return *data_begin(); }
    [[nodiscard]] constexpr const_reference front()    const { return *data_begin(); }
    [[nodiscard]] constexpr reference       back()           { return *(std::prev(data_end())); }
    [[nodiscard]] constexpr const_reference back()     const { return *(std::prev(data_end())); }

    /// Prepends a new element to the front of the container.
    ///
    /// Before the call to this function `size() < capacity()` must be true. Otherwise, the behavior is undefined.
    ///
    /// For `location::back` sequences, no iterators or references are invalidated, except `begin()`. For other
    /// sequences, all iterators and references are invalidated.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A reference to the inserted element.
    /// @exception Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::back` sequences
    ///            (strong exception guarantee), and leaves the sequence in a valid state for other sequence types
    ///            (basic exception guarantee).
    template <typename... Args>
    [[maybe_unused]] constexpr reference unchecked_emplace_front(Args&&... args)
    {
        if constexpr (sequence_traits::is_middle<Traits>) {
            if (storage_.first() == 0) {
                move_to_right((capacity() - size() + 1) / 2);
            }
        }
        if constexpr (sequence_traits::is_middle<Traits> || sequence_traits::is_back<Traits>) {
            ::new(std::prev(data_begin())) value_type(std::forward<Args>(args)...);
            storage_.first(storage_.first() - 1);
        } else {
            ::new(data_end()) value_type(std::forward<Args>(args)...);
            storage_.last(storage_.last() + 1);
            rotate_right(data_begin(), data_end());
        }
        return front();
    }

    /// Conditionally prepends a new element to the front of the container.
    ///
    /// If `size() == capacity()`, there are no effects. Otherwise, the new element is prepended.
    ///
    /// For `location::back` sequences, no iterators or references are invalidated, except `begin()`. For other
    /// sequences, all iterators and references are invalidated.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A pointer to the inserted element if `size() < capacity()`, `nullptr` otherwise.
    /// @exception Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::back` sequences
    ///            (strong exception guarantee), and leaves the sequence in a valid state for other sequence types
    ///            (basic exception guarantee).
    template <typename... Args>
    [[nodiscard]] constexpr pointer try_emplace_front(Args&&... args)
    {
        if (size() == capacity()) {
            return nullptr;
        }
        return std::addressof(unchecked_emplace_front(std::forward<Args>(args)...));
    }

    /// Prepends a new element to the front of the container.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A reference to the inserted element.
    /// @exception `std::bad_alloc` if additional capacity cannot be allocated.
    ///            Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::back` sequences
    ///            (strong exception guarantee), and leaves the sequence in a valid state for other sequence types
    ///            (basic exception guarantee).
    template <typename... Args>
    [[maybe_unused]] constexpr reference emplace_front(Args&&... args)
    {
        ensure_can_grow_by(1);
        return unchecked_emplace_front(std::forward<Args>(args)...);
    }

    /// Appends a new element to the back of the container.
    ///
    /// Before the call to this function `size() < capacity()` must be true. Otherwise, the behavior is undefined.
    ///
    /// For `location::front` sequences, no iterators or references are invalidated, except `end()`. For other
    /// sequences, all iterators and references are invalidated.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A reference to the inserted element.
    /// @exception Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::front` sequences
    ///            (strong exception guarantee), and leaves the sequence in a valid state for other sequence types
    ///            (basic exception guarantee).
    template <typename... Args>
    [[maybe_unused]] constexpr reference unchecked_emplace_back(Args&&... args)
    {
        if constexpr (sequence_traits::is_middle<Traits>) {
            if (storage_.last() == capacity()) {
                move_to_left((capacity() - size()) / 2);
            }
        }
        if constexpr (sequence_traits::is_front<Traits> || sequence_traits::is_middle<Traits>) {
            ::new(data_end()) value_type(std::forward<Args>(args)...);
            storage_.last(storage_.last() + 1);
        } else if constexpr (sequence_traits::is_back<Traits>) {
            ::new(std::prev(data_begin())) value_type(std::forward<Args>(args)...);
            storage_.first(storage_.first() - 1);
            rotate_left(data_begin(), data_end());
        }
        return back();
    }

    /// Conditionally appends a new element to the back of the container.
    ///
    /// If `size() == capacity()`, there are no effects. Otherwise, the new element is appended.
    ///
    /// For `location::front` sequences, no iterators or references are invalidated, except `end()`. For other
    /// sequences, all iterators and references are invalidated.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A pointer to the inserted element if `size() < capacity()`, `nullptr` otherwise.
    /// @exception Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::front` sequences
    ///            (strong exception guarantee), and leaves the sequence in a valid state for other sequence types
    ///            (basic exception guarantee).
    template <typename... Args>
    [[nodiscard]] constexpr pointer try_emplace_back(Args&&... args)
    {
        if (size() == capacity()) {
            return nullptr;
        }
        return std::addressof(unchecked_emplace_back(std::forward<Args>(args)...));
    }

    /// Append a new element to the back of the container.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A reference to the inserted element.
    /// @exception `std::bad_alloc` if additional capacity cannot be allocated.
    ///            Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::front` sequences
    ///            (strong exception guarantee), and leaves the sequence in a valid state for other sequence types
    ///            (basic exception guarantee).
    template <typename... Args>
    [[maybe_unused]] constexpr reference emplace_back(Args&&... args)
    {
        ensure_can_grow_by(1);
        return unchecked_emplace_back(std::forward<Args>(args)...);
    }

    /// Appends or prepends a new element to the container, using its native direction. `location::back` sequences will
    /// prepend to the front of the container, `location::front` and `location::middle` sequences will append to the
    /// back of the container.
    ///
    /// Before the call to this function `size() < capacity()` must be true. Otherwise, the behavior is undefined.
    ///
    /// For `location::front` sequences, the `end()` iterator is invalidated. For `location::back` sequences, the
    /// `begin()` iterator is invalidated. For `location::middle` sequences, all iterators and references are
    /// invalidated.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A reference to the inserted element.
    /// @exception Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::front` and
    ///            `location::back` sequences (strong exception guarantee), and leaves the sequence in a valid state for
    ///            `location::middle` sequences (basic exception guarantee).
    template <typename... Args>
    [[maybe_unused]] constexpr reference unchecked_emplace_native(Args&&... args)
    {
        if constexpr (sequence_traits::is_back<Traits>) {
            return unchecked_emplace_front(std::forward<Args>(args)...);
        } else {
            return unchecked_emplace_back(std::forward<Args>(args)...);
        }
    }

    /// Conditionally appends or prepends a new element to the container, using its native direction. `location::back`
    /// sequences will prepend to the front of the container, `location::front` and `location::middle` sequences will
    /// append to the back of the container.
    ///
    /// If `size() == capacity()`, there are no effects. Otherwise, the new element is appended.
    ///
    /// For `location::front` sequences, the `end()` iterator is invalidated. For `location::back` sequences, the
    /// `begin()` iterator is invalidated. For `location::middle` sequences, all iterators and references are
    /// invalidated.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A reference to the inserted element.
    /// @exception Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::front` and
    ///            `location::back` sequences (strong exception guarantee), and leaves the sequence in a valid state for
    ///            `location::middle` sequences (basic exception guarantee).
    template <typename... Args>
    [[nodiscard]] constexpr pointer try_emplace_native(Args&&... args)
    {
        if constexpr (sequence_traits::is_back<Traits>) {
            return try_emplace_front(std::forward<Args>(args)...);
        } else {
            return try_emplace_back(std::forward<Args>(args)...);
        }
    }

    /// Appends or prepends a new element to the container, using its native direction. `location::back` sequences will
    /// prepend to the front of the container, `location::front` and `location::middle` sequences will append to the
    /// back of the container.
    ///
    /// @param args Arguments to forward to the constructor of the element. 
    /// @return A reference to the inserted element.
    /// @exception `std::bad_alloc` if additional capacity cannot be allocated.
    ///            Any exception thrown by initialization of the inserted element, or while moving other elements. If an
    ///            exception is thrown for any reason, this function has no effect for `location::front` and
    ///            `location::back` sequences (strong exception guarantee), and leaves the sequence in a valid state for
    ///            `location::middle` sequences (basic exception guarantee).
    template <typename... Args>
    [[maybe_unused]] constexpr reference emplace_native(Args&&... args)
    {
        if constexpr (sequence_traits::is_back<Traits>) {
            return emplace_front(std::forward<Args>(args)...);
        } else {
            return emplace_back(std::forward<Args>(args)...);
        }
    }

    constexpr void pop_front()
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

    constexpr void pop_back()
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
    using range_guard = detail::range_guard<T>;
    using storage_type = detail::storage<T, Traits>;

    [[nodiscard]] constexpr pointer       data_begin()               { return storage_.data(storage_.first()); }
    [[nodiscard]] constexpr const_pointer data_begin()         const { return storage_.data(storage_.first()); }
    [[nodiscard]] constexpr pointer       data_end()                 { return storage_.data(storage_.last()); }
    [[nodiscard]] constexpr const_pointer data_end()           const { return storage_.data(storage_.last()); }
    [[nodiscard]] constexpr pointer       data_at(size_type i)       { return storage_.data(i); }
    [[nodiscard]] constexpr const_pointer data_at(size_type i) const { return storage_.data(i); }

    [[nodiscard]] constexpr size_type data_index(const_pointer pos) const
    {
        return static_cast<size_type>(std::distance(storage_.data(0), pos));
    }

    constexpr void destroy_all() { std::ranges::destroy(begin(), end()); }

    constexpr void ensure_can_grow_by(difference_type n)
    {
        if (static_cast<difference_type>(capacity() - size()) < n) {
            if constexpr (sequence_traits::is_variable<Traits>) {
                static_assert(false, "Not implemented");
            } else {
                throw std::bad_alloc();
            }
        }
    }

    constexpr void rotate_left(pointer from, pointer to)
    {
        if (std::distance(from, to) > 1) {
            value_type tmp{std::move(*from)};
            std::move(std::next(from), to, from);
            *std::prev(to) = std::move(tmp);
        }
    }

    constexpr void rotate_right(pointer from, pointer to)
    {
        if (std::distance(from, to) > 1) {
            value_type tmp{std::move(*std::prev(to))};
            std::move_backward(from, std::prev(to), to);
            *from = std::move(tmp);
        }
    }

    constexpr void move_to_left(size_type new_first) requires sequence_traits::is_middle<Traits>
    {
        const auto uninitialized_available = static_cast<size_type>(storage_.first() - new_first);
        const auto uninitialized_size = std::min(uninitialized_available, size());
        const auto to = std::next(data_begin(), uninitialized_size);
        auto guarded = uninitialized_move(data_begin(), to, data_at(new_first));
        const auto old_end = data_end();
        const auto last = std::move(to, old_end, guarded.last);
        storage_.first(data_index(guarded.first));
        storage_.last(data_index(last));
        guarded.release();
        std::ranges::destroy(last, old_end);
    }

    constexpr void move_to_right(size_type new_first) requires sequence_traits::is_middle<Traits>
    {
        const auto uninitialized_available = static_cast<size_type>(new_first - storage_.first());
        const auto uninitialized_size = std::min(uninitialized_available, size());
        const auto uninitialized_end = data_at(new_first + size());
        const auto from = std::prev(data_end(), uninitialized_size);
        auto guarded = uninitialized_move(from, data_end(), std::prev(uninitialized_end, uninitialized_size));
        const auto old_begin = data_begin();
        const auto first = std::move_backward(old_begin, from, guarded.first);
        storage_.first(data_index(first));
        storage_.last(data_index(guarded.last));
        guarded.release();
        std::ranges::destroy(old_begin, first);
    }

    [[nodiscard]] constexpr range_guard uninitialized_move(pointer from, pointer to, pointer dst)
    {
        range_guard range{dst, dst};
        for (; from != to; ++range.last, ++from) {
            ::new(range.last) value_type(std::move(*from));
        }
        return range;
    }

    static inline constexpr traits_type traits_{Traits};

    [[no_unique_address]] storage_type storage_;
};

template <typename T>
class sequence<T> : public sequence<T, sequence_traits::traits_t{}>
{
public:
    using sequence<T, sequence_traits::traits_t{}>::sequence;
};

template <typename T, sequence_traits::specifier_t auto... Specifiers>
class sequence<T, Specifiers...> : public sequence<T, sequence_traits::make_traits(Specifiers...)>
{
public:
    using sequence<T, sequence_traits::make_traits(Specifiers...)>::sequence;
};

} // namespace jell
