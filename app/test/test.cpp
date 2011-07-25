#define BOOST_TEST_MODULE GringoTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( fail )
{
	BOOST_CHECK(false == true);
}
