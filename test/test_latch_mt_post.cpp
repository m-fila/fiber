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

void wait_fn( boost::fibers::latch & l) {
    l.wait();
    ++value;
}

void count_down_fn( boost::fibers::latch & l) {
    l.count_down();
}

void arrive_and_wait_fn( boost::fibers::latch & l) {
    l.arrive_and_wait();
    ++value;
}

void test_latch() {
    value = 0;

    boost::fibers::latch l( 3);
    BOOST_CHECK( !l.try_wait() );

    boost::thread t1([&] {
        boost::fibers::fiber(
            boost::fibers::launch::post,
            wait_fn,
            std::ref( l)).join();
    });

    boost::thread t2([&] {
        boost::fibers::fiber(
            boost::fibers::launch::post,
            count_down_fn,
            std::ref( l)).join();
    });

    boost::thread t3([&] {
        boost::fibers::fiber(
            boost::fibers::launch::post,
            arrive_and_wait_fn,
            std::ref( l)).join();
    });

    boost::this_fiber::yield();
    
    BOOST_CHECK_EQUAL( 0, value);
    BOOST_CHECK( !l.try_wait() );

    l.count_down();

    t1.join();
    t2.join();
    t3.join();

    BOOST_CHECK( l.try_wait() );
    BOOST_CHECK_EQUAL( 2, value);
}

void test_dummy() {}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE(
            "Boost.Fiber: multithreaded latch test suite");

#if ! defined(BOOST_FIBERS_NO_ATOMICS)
    test->add( BOOST_TEST_CASE( & test_latch) );
#else
    test->add( BOOST_TEST_CASE( & test_dummy) );
#endif
    return test;
}
