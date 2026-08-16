
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

semaphore::semaphore( std::size_t initial) : current_{ initial } {}

void semaphore::count_down( std::size_t n) {
    BOOST_ASSERT( n > 0);
    std::unique_lock<mutex> lk{ mtx_ };
    BOOST_ASSERT(n <= current_);
    current_ -= n;
    if (0 == current_) {
        lk.unlock(); // no pessimization
        cond_.notify_all();
    }
}

bool semaphore::try_wait() const {
    std::unique_lock<mutex> lk{ mtx_ };
    return 0 == current_;
}

void semaphore::wait() {
    std::unique_lock<mutex> lk{ mtx_ };
    cond_.wait( lk, [this]() { return 0 == current_; });
}

void semaphore::arrive_and_wait( std::size_t n) {
    BOOST_ASSERT( n > 0);
    std::unique_lock<mutex> lk{ mtx_ };
    BOOST_ASSERT( n <= current_);
    current_ -= n;
    if ( 0 == current_) {
        lk.unlock(); // no pessimization
        cond_.notify_all();
    } else {
        cond_.wait( lk, [this]() { return 0 == current_; });
    }
}

} // namespace fibers
} // namespace boost

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif
