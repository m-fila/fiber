
//          Copyright Mateusz Jakub Fila 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt) 

#ifndef BOOST_FIBERS_SEMAPHORE_H
#define BOOST_FIBERS_SEMAPHORE_H

#include <chrono>
#include <cstddef>
#include <limits>

#include <boost/config.hpp>

#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/convert.hpp>
#include <boost/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL semaphore {
private:
    std::size_t         count_;
    mutable mutex       mtx_{};
    condition_variable  cond_{};

    bool try_acquire_until_( std::chrono::steady_clock::time_point const& timeout_time);

public:
    static constexpr std::size_t max() noexcept {
        return ( std::numeric_limits< std::size_t >::max)();
    }

    explicit semaphore( std::size_t initial = 0);

    semaphore( semaphore const&) = delete;
    semaphore & operator=( semaphore const&) = delete;
    semaphore( semaphore &&) = delete;
    semaphore & operator=( semaphore &&) = delete;
    ~semaphore() = default;

    void release( std::size_t n = 1);

    void acquire();

    bool try_acquire();

    template< typename Clock, typename Duration >
    bool try_acquire_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        std::chrono::steady_clock::time_point timeout_time = detail::convert( timeout_time_);
        return try_acquire_until_( timeout_time);
    }

    template< typename Rep, typename Period >
    bool try_acquire_for( std::chrono::duration< Rep, Period > const& timeout_duration) {
        return try_acquire_until_( std::chrono::steady_clock::now() + timeout_duration);
    }
};

class BOOST_FIBERS_DECL binary_semaphore {
private:
    bool                available_;
    mutable mutex       mtx_{};
    condition_variable  cond_{};

    bool try_acquire_until_( std::chrono::steady_clock::time_point const& timeout_time);

public:
    static constexpr std::size_t max() noexcept {
        return 1;
    }

    explicit binary_semaphore( bool initial = false);

    binary_semaphore( binary_semaphore const&) = delete;
    binary_semaphore & operator=( binary_semaphore const&) = delete;
    binary_semaphore( binary_semaphore &&) = delete;
    binary_semaphore & operator=( binary_semaphore &&) = delete;
    ~binary_semaphore() = default;

    void release();

    void acquire();

    bool try_acquire();

    template< typename Clock, typename Duration >
    bool try_acquire_until( std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        std::chrono::steady_clock::time_point timeout_time = detail::convert( timeout_time_);
        return try_acquire_until_( timeout_time);
    }

    template< typename Rep, typename Period >
    bool try_acquire_for( std::chrono::duration< Rep, Period > const& timeout_duration) {
        return try_acquire_until_( std::chrono::steady_clock::now() + timeout_duration);
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_SEMAPHORE_H
