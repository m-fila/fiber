//          Copyright Mateusz Jakub Fila 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/atomic.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

#include <boost/fiber/all.hpp>

boost::atomic< int > value;

void acquire_fn( boost::fibers::semaphore & s) {
    s.acquire();
    ++value;
}

void acquire_binary_fn( boost::fibers::binary_semaphore & s) {
    s.acquire();
    ++value;
}

void test_semaphore() {
    value = 0;

    boost::fibers::semaphore s( 0);

    boost::thread t1([&] {
        boost::fibers::fiber(
            boost::fibers::launch::dispatch,
            acquire_fn,
            std::ref( s)).join();
    });

    boost::thread t2([&] {
        boost::fibers::fiber(
            boost::fibers::launch::dispatch,
            acquire_fn,
            std::ref( s)).join();
    });

    boost::this_fiber::yield();
    s.release( 2);

    t1.join();
    t2.join();

    BOOST_CHECK( ! s.try_acquire() );
}

void test_binary_semaphore() {
    value = 0;

    boost::fibers::binary_semaphore s( false);

    boost::thread t1([&] {
        boost::fibers::fiber(
            boost::fibers::launch::dispatch,
            acquire_binary_fn,
            std::ref( s)).join();
    });

    boost::thread t2([&] {
        boost::fibers::fiber(
            boost::fibers::launch::dispatch,
            acquire_binary_fn,
            std::ref( s)).join();
    });

    // release() requires the permit to be unavailable, so wait for each acquire
    s.release();
    while ( 1 != value) {
        boost::this_fiber::yield();
    }
    s.release();

    t1.join();
    t2.join();

    BOOST_CHECK_EQUAL( 2, value);
    BOOST_CHECK( ! s.try_acquire() );
}

void test_dummy() {}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE(
            "Boost.Fiber: multithreaded semaphore test suite");

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    test->add( BOOST_TEST_CASE( & test_semaphore) );
    test->add( BOOST_TEST_CASE( & test_binary_semaphore) );
#else
    test->add( BOOST_TEST_CASE( & test_dummy) );
#endif
    return test;
}
