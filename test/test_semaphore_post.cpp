//          Copyright Mateusz Jakub Fila 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#include <chrono>

#include <boost/test/unit_test.hpp>

#include <boost/fiber/all.hpp>

int value = 0;

void test_semaphore() {
    value = 0;

    boost::fibers::semaphore s( 0);
    BOOST_CHECK( ! s.try_acquire() );

    boost::fibers::fiber f1(
        boost::fibers::launch::post,
        [&] {
            s.acquire();
            ++value;
        });
    boost::fibers::fiber f2(
        boost::fibers::launch::post,
        [&] {
            s.acquire();
            ++value;
        });

    boost::this_fiber::yield();
    BOOST_CHECK_EQUAL( 0, value);

    s.release( 2);
    f1.join();
    f2.join();

    BOOST_CHECK_EQUAL( 2, value);
    BOOST_CHECK( ! s.try_acquire() );
}

void test_semaphore_initial() {
    boost::fibers::semaphore s( 2);
    BOOST_CHECK( s.try_acquire() );
    BOOST_CHECK( s.try_acquire() );
    BOOST_CHECK( ! s.try_acquire() );
    s.release();
    BOOST_CHECK( s.try_acquire() );
}

void test_semaphore_timed() {
    boost::fibers::semaphore s( 0);
    BOOST_CHECK( ! s.try_acquire_for( std::chrono::milliseconds( 20) ) );
    BOOST_CHECK( ! s.try_acquire_until( std::chrono::steady_clock::now() + std::chrono::milliseconds( 20) ) );
    s.release();
    BOOST_CHECK( s.try_acquire_for( std::chrono::milliseconds( 20) ) );
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: semaphore test suite");

    test->add( BOOST_TEST_CASE( & test_semaphore) );
    test->add( BOOST_TEST_CASE( & test_semaphore_initial) );
    test->add( BOOST_TEST_CASE( & test_semaphore_timed) );

    return test;
}
