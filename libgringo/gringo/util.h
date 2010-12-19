#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/tuple/tuple.hpp>


// too convienient to not include :)
namespace _boost
{
	namespace range_detail
	{
		template< class Rng1, class Rng2 >
		struct zip_range
			: ::boost::iterator_range<
				::boost::zip_iterator<
					::boost::tuples::tuple<
						BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng1>::type,
						BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng2>::type
					>
				>
			>
		{
		private:
			typedef ::boost::zip_iterator<
				::boost::tuples::tuple<
					BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng1>::type,
					BOOST_DEDUCED_TYPENAME ::boost::range_iterator<Rng2>::type
				>
			> zip_iter_t;
			typedef ::boost::iterator_range<zip_iter_t> base_t;

		public:
			zip_range( Rng1& r1, Rng2& r2 )
				: base_t( zip_iter_t( ::boost::tuples::make_tuple(::boost::begin(r1),
				::boost::begin(r2)) ),
				zip_iter_t( ::boost::tuples::make_tuple(::boost::end(r1),
				::boost::end(r2)) ) )
			{
				BOOST_ASSERT(::boost::distance(r1) <= ::boost::distance(r2));
			}
		};
	}
	
	template< class Rng1, class Rng2 >
	inline ::_boost::range_detail::zip_range<Rng1, Rng2> combine( Rng1& r1, Rng2& r2 )
	{
		return ::_boost::range_detail::zip_range<Rng1, Rng2>(r1, r2);
	}
}

