
//          Copyright Mateusz Jakub Fila 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/semaphore.hpp"

#include <cstddef>

#include <boost/assert.hpp>

#include "boost/fiber/context.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

semaphore::semaphore( std::size_t initial) :
    count_{ initial } {
}

void
semaphore::release( std::size_t n) {
    detail::spinlock_lock lk{ splk_ };
    // hand permits directly to waiters, bank the remainder
    while ( 0 != n && wait_queue_.notify_one()) {
        --n;
    }
    BOOST_ASSERT( n <= max() - count_);
    count_ += n;
}

void
semaphore::acquire() {
    context * active_ctx = context::active();
    detail::spinlock_lock lk{ splk_ };
    if ( 0 != count_) {
        --count_;
        return;
    }
    // returns only once release() has handed this fiber a permit
    wait_queue_.suspend_and_wait( lk, active_ctx);
}

bool
semaphore::try_acquire() {
    detail::spinlock_lock lk{ splk_ };
    if ( 0 == count_) {
        return false;
    }
    --count_;
    return true;
}

bool
semaphore::try_acquire_until_( std::chrono::steady_clock::time_point const& timeout_time) {
    context * active_ctx = context::active();
    detail::spinlock_lock lk{ splk_ };
    if ( 0 != count_) {
        --count_;
        return true;
    }
    if ( std::chrono::steady_clock::now() > timeout_time) {
        return false;
    }
    return wait_queue_.suspend_and_wait_until( lk, active_ctx, timeout_time);
}

binary_semaphore::binary_semaphore( bool initial) :
    available_{ initial } {
}

void
binary_semaphore::release() {
    detail::spinlock_lock lk{ splk_ };
    BOOST_ASSERT( ! available_);
    if ( ! wait_queue_.notify_one()) {
        available_ = true;
    }
}

void
binary_semaphore::acquire() {
    context * active_ctx = context::active();
    detail::spinlock_lock lk{ splk_ };
    if ( available_) {
        available_ = false;
        return;
    }
    // returns only once release() has handed this fiber the permit
    wait_queue_.suspend_and_wait( lk, active_ctx);
}

bool
binary_semaphore::try_acquire() {
    detail::spinlock_lock lk{ splk_ };
    if ( ! available_) {
        return false;
    }
    available_ = false;
    return true;
}

bool
binary_semaphore::try_acquire_until_( std::chrono::steady_clock::time_point const& timeout_time) {
    context * active_ctx = context::active();
    detail::spinlock_lock lk{ splk_ };
    if ( available_) {
        available_ = false;
        return true;
    }
    if ( std::chrono::steady_clock::now() > timeout_time) {
        return false;
    }
    return wait_queue_.suspend_and_wait_until( lk, active_ctx, timeout_time);
}

} // namespace fibers
} // namespace boost

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif
