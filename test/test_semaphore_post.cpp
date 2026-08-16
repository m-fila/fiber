
//          Copyright Mateusz Jakub Fila 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include <boost/fiber/all.hpp>

int value = 0;

void test_semaphore() {
    value = 0;

    boost::fibers::semaphore l( 3);
    BOOST_CHECK( !l.try_wait() );

    boost::fibers::fiber f1(
        boost::fibers::launch::post,
        [&] {
            l.wait();
            value = 1;
        });

    boost::this_fiber::yield();

    BOOST_CHECK( !l.try_wait() );
    BOOST_CHECK_EQUAL( 0, value);

    l.count_down( 2);
    l.count_down( 1);
    BOOST_CHECK( l.try_wait() );

    f1.join();

    BOOST_CHECK_EQUAL( 1, value);
}

void test_semaphore_arrive_and_wait() {
    value = 0;

    boost::fibers::semaphore l( 6);
    BOOST_CHECK( !l.try_wait() );

    boost::fibers::fiber f1(
        boost::fibers::launch::post,
        [&] {
            ++value;
            l.arrive_and_wait( 1);
            ++value;
        });

    boost::this_fiber::yield();
    BOOST_CHECK( !l.try_wait() );
    BOOST_CHECK_LT( value, 2);

    boost::fibers::fiber f2(
        boost::fibers::launch::post,
        [&] {
            ++value;
            l.arrive_and_wait( 2);
            ++value;
        });

    boost::this_fiber::yield();
    BOOST_CHECK( !l.try_wait() );
    BOOST_CHECK_LT( value, 3);

    l.count_down( 2);
    l.count_down( 1);
    BOOST_CHECK( l.try_wait() );

    f1.join();
    f2.join();

    BOOST_CHECK_EQUAL( 4, value);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Fiber: semaphore test suite");

    test->add( BOOST_TEST_CASE( & test_semaphore) );
    test->add( BOOST_TEST_CASE( & test_semaphore_arrive_and_wait) );

    return test;
}
