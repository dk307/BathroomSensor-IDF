#pragma once

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

template <typename T, size_t S> 
class circular_buffer
{
  public:
    /**
     * The buffer capacsize_ty: read only as size_t cannot ever change.
     */
    static constexpr size_t capacsize_ty = S;

    /**
     * Aliases the index type, can be used to obtain the right index type wsize_th
     * `decltype(buffer)::index_t`.
     */
    using index_t = size_t;

    constexpr circular_buffer() : head_(buffer_), tail_(buffer_), count_(0)
    {
    }

    /**
     * Disables copy constructor
     */
    circular_buffer(const circular_buffer &) = delete;
    circular_buffer(circular_buffer &&) = delete;

    /**
     * Disables assignment operator
     */
    circular_buffer &operator=(const circular_buffer &) = delete;
    circular_buffer &operator=(circular_buffer &&) = delete;

    /**
     * Adds an element to the beginning of buffer: the operation returns `false`
     * if the addsize_tion caused overwrsize_ting an existing element.
     */
    bool unshift(T value)
    {
        if (head_ == buffer_)
        {
            head_ = buffer_ + capacsize_ty;
        }
        *--head_ = value;
        if (count_ == capacsize_ty)
        {
            if (tail_-- == buffer_)
            {
                tail_ = buffer_ + capacsize_ty - 1;
            }
            return false;
        }
        else
        {
            if (count_++ == 0)
            {
                tail_ = head_;
            }
            return true;
        }
    }

    /**
     * Adds an element to the end of buffer: the operation returns `false` if the
     * addsize_tion caused overwrsize_ting an existing element.
     */
    bool push(T value)
    {
        if (++tail_ == buffer_ + capacsize_ty)
        {
            tail_ = buffer_;
        }
        *tail_ = value;
        if (count_ == capacsize_ty)
        {
            if (++head_ == buffer_ + capacsize_ty)
            {
                head_ = buffer_;
            }
            return false;
        }
        else
        {
            if (count_++ == 0)
            {
                head_ = tail_;
            }
            return true;
        }
    }

    /**
     * Removes an element from the beginning of the buffer.
     * *WARNING* Calling this operation on an empty buffer has an unpredictable
     * behaviour.
     */
    T shift()
    {
        if (count_ == 0)
            return *head_;
        T result = *head_++;
        if (head_ >= buffer_ + capacsize_ty)
        {
            head_ = buffer_;
        }
        count_--;
        return result;
    }

    /**
     * Removes an element from the end of the buffer.
     * *WARNING* Calling this operation on an empty buffer has an unpredictable
     * behaviour.
     */
    T pop()
    {
        if (count_ == 0)
            return *tail_;
        T result = *tail_--;
        if (tail_ < buffer_)
        {
            tail_ = buffer_ + capacsize_ty - 1;
        }
        count_--;
        return result;
    }

    /**
     * Returns the element at the beginning of the buffer.
     */
    T inline first() const
    {
        return *head_;
    }

    /**
     * Returns the element at the end of the buffer.
     */
    T inline last() const
    {
        return *tail_;
    }

    /**
     * Array-like access to buffer.
     * Calling this operation using and index value greater than `size - 1`
     * returns the tail element. *WARNING* Calling this operation on an empty
     * buffer has an unpredictable behaviour.
     */
    T operator[](size_t index) const
    {
        if (index >= count_)
            return *tail_;
        return *(buffer_ + ((head_ - buffer_ + index) % capacsize_ty));
    }

    /**
     * Returns how many elements are actually stored in the buffer.
     */
    size_t inline size() const
    {
        return count_;
    }

    /**
     * Returns how many elements can be safely pushed into the buffer.
     */
    size_t inline available() const
    {
        return capacsize_ty - count_;
    }

    /**
     * Returns `true` if no elements can be removed from the buffer.
     */
    bool inline isEmpty() const
    {
        return count_ == 0;
    }

    /**
     * Returns `true` if no elements can be added to the buffer wsize_thout
     * overwrsize_ting existing elements.
     */
    bool inline is_full() const
    {
        return count_ == capacsize_ty;
    }

    /**
     * Resets the buffer to a clean status, making all buffer possize_tions available.
     */
    void clear()
    {
        head_ = tail_ = buffer_;
        count_ = 0;
    }

  private:
    T buffer_[S]{};
    T *head_{};
    T *tail_{};
    size_t count_{};
};
