
//          Copyright Mateusz Jakub Fila 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt) 

#ifndef BOOST_FIBERS_SEMAPHORE_H
#define BOOST_FIBERS_SEMAPHORE_H

#include <cstddef>

#include <boost/config.hpp>

#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class BOOST_FIBERS_DECL semaphore {
private:
    std::size_t         current_;
    mutable mutex       mtx_{};
    condition_variable  cond_{};

public:
    explicit semaphore( std::size_t);

    semaphore( semaphore const&) = delete;
    semaphore & operator=( semaphore const&) = delete;
    semaphore( semaphore &&) = delete;
    semaphore & operator=( semaphore &&) = delete;
    ~semaphore() = default;

    void count_down( std::size_t n = 1);
    bool try_wait() const;
    void wait();
    void arrive_and_wait( std::size_t n = 1);
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_SEMAPHORE_H
