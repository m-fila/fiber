
//          Copyright Mateusz Jakub Fila 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/fiber/semaphore.hpp"

#include <cstddef>
#include <mutex>

#include <boost/assert.hpp>

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
    if ( 0 == n) {
        return;
    }
    std::unique_lock< mutex > lk{ mtx_ };
    BOOST_ASSERT( n <= max() - count_);
    count_ += n;
    lk.unlock();
    if ( 1 == n) {
        cond_.notify_one();
    } else {
        cond_.notify_all();
    }
}

void semaphore::acquire() {
    std::unique_lock< mutex > lk{ mtx_ };
    cond_.wait( lk, [this]{ return 0 != count_; });
    --count_;
}

bool semaphore::try_acquire() {
    std::unique_lock< mutex > lk{ mtx_ };
    if ( 0 == count_) {
        return false;
    }
    --count_;
    return true;
}

bool semaphore::try_acquire_until_( std::chrono::steady_clock::time_point const& timeout_time) {
    std::unique_lock< mutex > lk{ mtx_ };
    if ( ! cond_.wait_until( lk, timeout_time, [this]{ return 0 != count_; }) ) {
        return false;
    }
    --count_;
    return true;
}

binary_semaphore::binary_semaphore( bool initial) :
    available_{ initial } {
}

void binary_semaphore::release() {
    std::unique_lock< mutex > lk{ mtx_ };
    BOOST_ASSERT( ! available_);
    available_ = true;
    lk.unlock();
    cond_.notify_one();
}

void binary_semaphore::acquire() {
    std::unique_lock< mutex > lk{ mtx_ };
    cond_.wait( lk, [this]{ return available_; });
    available_ = false;
}

bool binary_semaphore::try_acquire() {
    std::unique_lock< mutex > lk{ mtx_ };
    if ( ! available_) {
        return false;
    }
    available_ = false;
    return true;
}

bool binary_semaphore::try_acquire_until_( std::chrono::steady_clock::time_point const& timeout_time) {
    std::unique_lock< mutex > lk{ mtx_ };
    if ( ! cond_.wait_until( lk, timeout_time, [this]{ return available_; }) ) {
        return false;
    }
    available_ = false;
    return true;
}

} // namespace fibers
} // namespace boost

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif
