#ifndef BOOST_FIBERS_UNBOUND_CHANNEL_H
#define BOOST_FIBERS_UNBOUND_CHANNEL_H

#include <chrono>
#include <cstddef>
#include <deque>
#include <iterator>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

#include <boost/config.hpp>

#include <boost/fiber/channel_op_status.hpp>
#include <boost/fiber/context.hpp>
#include <boost/fiber/waker.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/detail/spinlock.hpp>
#include <boost/fiber/exceptions.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

// Unbounded channel: push never suspends (storage grows), pop suspends while empty.
template< typename T, typename Allocator = std::allocator< typename std::remove_reference< T >::type > >
class unbound_channel {
public:
    using value_type = typename std::remove_reference< T >::type;
    using allocator_type = Allocator;

private:
    using queue_type = std::deque< value_type, Allocator >;

    mutable detail::spinlock    splk_{};
    wait_queue                  waiting_consumers_{};
    queue_type                  queue_;
    bool                        closed_{ false };

    bool is_empty_() const noexcept {
        return queue_.empty();
    }

    bool is_closed_() const noexcept {
        return closed_;
    }

    void lock_( detail::spinlock_lock & lk) const {
        for (;;) {
            if ( lk.try_lock() ) {
                break;
            }
            context::active()->yield();
        }
    }

    template< typename V >
    channel_op_status push_( V && value) {
        detail::spinlock_lock lk{ splk_, std::defer_lock };
        lock_( lk);
        if ( BOOST_UNLIKELY( is_closed_() ) ) {
            return channel_op_status::closed;
        }
        queue_.push_back( std::forward< V >( value) );
        waiting_consumers_.notify_one();
        return channel_op_status::success;
    }

public:
    unbound_channel() = default;

    explicit unbound_channel( Allocator const& alloc) :
        queue_( alloc) {
    }

    ~unbound_channel() {
        close();
    }

    unbound_channel( unbound_channel const&) = delete;
    unbound_channel & operator=( unbound_channel const&) = delete;

    allocator_type get_allocator() const {
        return queue_.get_allocator();
    }

    bool is_closed() const noexcept {
        detail::spinlock_lock lk{ splk_, std::defer_lock };
        lock_( lk);
        return is_closed_();
    }

    void close() noexcept {
        detail::spinlock_lock lk{ splk_, std::defer_lock };
        lock_( lk);
        if ( ! closed_) {
            closed_ = true;
            waiting_consumers_.notify_all();
        }
    }

    channel_op_status push( value_type const& value) {
        return push_( value);
    }

    channel_op_status push( value_type && value) {
        return push_( std::move( value) );
    }

    channel_op_status try_pop( value_type & value) {
        detail::spinlock_lock lk{ splk_, std::defer_lock };
        lock_( lk);
        if ( is_empty_() ) {
            return is_closed_()
                ? channel_op_status::closed
                : channel_op_status::empty;
        }
        value = std::move( queue_.front() );
        queue_.pop_front();
        return channel_op_status::success;
    }

    channel_op_status pop( value_type & value) {
        context * active_ctx = context::active();
        for (;;) {
            detail::spinlock_lock lk{ splk_, std::try_to_lock };
            if ( ! lk) {
                active_ctx->yield();
                continue;
            }
            if ( is_empty_() ) {
                if ( BOOST_UNLIKELY( is_closed_() ) ) {
                    return channel_op_status::closed;
                }
                waiting_consumers_.suspend_and_wait( lk, active_ctx);
            } else {
                value = std::move( queue_.front() );
                queue_.pop_front();
                return channel_op_status::success;
            }
        }
    }

    value_type value_pop() {
        context * active_ctx = context::active();
        for (;;) {
            detail::spinlock_lock lk{ splk_, std::try_to_lock };
            if ( ! lk) {
                active_ctx->yield();
                continue;
            }
            if ( is_empty_() ) {
                if ( BOOST_UNLIKELY( is_closed_() ) ) {
                    throw fiber_error{
                        std::make_error_code( std::errc::operation_not_permitted),
                        "boost fiber: channel is closed" };
                }
                waiting_consumers_.suspend_and_wait( lk, active_ctx);
            } else {
                value_type value = std::move( queue_.front() );
                queue_.pop_front();
                return value;
            }
        }
    }

    template< typename Rep, typename Period >
    channel_op_status pop_wait_for( value_type & value,
                                    std::chrono::duration< Rep, Period > const& timeout_duration) {
        return pop_wait_until( value,
                               std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename Clock, typename Duration >
    channel_op_status pop_wait_until( value_type & value,
                                      std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        context * active_ctx = context::active();
        std::chrono::steady_clock::time_point timeout_time = detail::convert( timeout_time_);
        for (;;) {
            detail::spinlock_lock lk{ splk_, std::try_to_lock };
            if ( ! lk) {
                active_ctx->yield();
                continue;
            }
            if ( is_empty_() ) {
                if ( BOOST_UNLIKELY( is_closed_() ) ) {
                    return channel_op_status::closed;
                }
                if ( ! waiting_consumers_.suspend_and_wait_until( lk, active_ctx, timeout_time) ) {
                    return channel_op_status::timeout;
                }
            } else {
                value = std::move( queue_.front() );
                queue_.pop_front();
                return channel_op_status::success;
            }
        }
    }

    class iterator {
    private:
        typedef unsigned char storage_type[sizeof(value_type)];

        unbound_channel *                chan_{ nullptr };
        alignas(value_type) storage_type storage_;

        void increment_( bool initial = false) {
            BOOST_ASSERT( nullptr != chan_);
            try {
                if ( ! initial) {
                    reinterpret_cast< value_type * >( std::addressof( storage_) )->~value_type();
                }
                ::new ( static_cast< void * >( std::addressof( storage_) ) ) value_type{ chan_->value_pop() };
            } catch ( fiber_error const&) {
                chan_ = nullptr;
            }
        }

    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        using pointer_t = pointer;
        using reference_t = reference;

        iterator() = default;

        explicit iterator( unbound_channel * chan) noexcept :
            chan_{ chan } {
            increment_( true);
        }

        iterator( iterator const& other) noexcept :
            chan_{ other.chan_ } {
        }

        iterator & operator=( iterator const& other) noexcept {
            if ( BOOST_LIKELY( this != & other) ) {
                chan_ = other.chan_;
            }
            return * this;
        }

        bool operator==( iterator const& other) const noexcept {
            return other.chan_ == chan_;
        }

        bool operator!=( iterator const& other) const noexcept {
            return other.chan_ != chan_;
        }

        iterator & operator++() {
            increment_();
            return * this;
        }

        const iterator operator++( int) = delete;

        reference_t operator*() noexcept {
            return * reinterpret_cast< value_type * >( std::addressof( storage_) );
        }

        pointer_t operator->() noexcept {
            return reinterpret_cast< value_type * >( std::addressof( storage_) );
        }
    };

    friend class iterator;
};

template< typename T, typename Allocator >
typename unbound_channel< T, Allocator >::iterator
begin( unbound_channel< T, Allocator > & chan) {
    return typename unbound_channel< T, Allocator >::iterator( & chan);
}

template< typename T, typename Allocator >
typename unbound_channel< T, Allocator >::iterator
end( unbound_channel< T, Allocator > &) {
    return typename unbound_channel< T, Allocator >::iterator();
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_UNBOUND_CHANNEL_H
