#include "doctest.h"
#include <emergent/Maths.hpp>


TEST_CASE("finding the means of values")
{
	SUBCASE("an empty vector returns zero")
	{
		CHECK(emg::Maths::mean<int>({}) == 0);
	}

	SUBCASE("a single value returns that value")
	{
		CHECK(emg::Maths::mean<int>({ 42 }) == 42);
	}

	SUBCASE("mean is calculated for multiple values")
	{
		CHECK(emg::Maths::mean<int>({ 42, 13 }) == 27.5);
	}
}


TEST_CASE("finding the median of values")
{
	SUBCASE("an empty vector returns zero")
	{
		std::vector<int> empty;

		CHECK(emg::Maths::median(empty) == 0);
	}

	SUBCASE("a single value returns that value")
	{
		std::vector<int> single	= { 42 };
		CHECK(emg::Maths::median(single) == 42);
	}

	SUBCASE("two values return the mean")
	{
		std::vector<int> pair			= { 42, 13 };
		std::vector<double> floating	= { 42.0, 13.0 };

		CHECK(emg::Maths::median(pair) == 27);
		CHECK(emg::Maths::median(floating) == 27.5);
	}

	SUBCASE("odd number of values return the median")
	{
		std::vector<int> odd = { 42, 5, 12, 93, -1 };
		CHECK(emg::Maths::median(odd) == 12);
	}

	SUBCASE("even number of values return the larger of the two median values")
	{
		std::vector<int> even = { 42, 5, 12, 93 };
		CHECK(emg::Maths::median(even) == 42);
	}
}
