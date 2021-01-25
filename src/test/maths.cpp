#include <catch.hpp>
#include <emergent/Maths.hpp>


TEST_CASE("finding the means of values", "[maths]")
{
	SECTION("an empty vector returns zero")
	{
		REQUIRE(emg::Maths::mean<int>({}) == 0);
	}

	SECTION("a single value returns that value")
	{
		REQUIRE(emg::Maths::mean<int>({ 42 }) == 42);
	}

	SECTION("mean is calculated for multiple values")
	{
		REQUIRE(emg::Maths::mean<int>({ 42, 13 }) == 27.5);
	}
}


TEST_CASE("finding the median of values", "[maths]")
{
	SECTION("an empty vector returns zero")
	{
		std::vector<int> empty;

		REQUIRE(emg::Maths::median(empty) == 0);
	}

	SECTION("a single value returns that value")
	{
		std::vector<int> single	= { 42 };
		REQUIRE(emg::Maths::median(single) == 42);
	}

	SECTION("two values return the mean")
	{
		std::vector<int> pair			= { 42, 13 };
		std::vector<double> floating	= { 42.0, 13.0 };

		REQUIRE(emg::Maths::median(pair) == 27);
		REQUIRE(emg::Maths::median(floating) == 27.5);
	}

	SECTION("odd number of values return the median")
	{
		std::vector<int> odd = { 42, 5, 12, 93, -1 };
		REQUIRE(emg::Maths::median(odd) == 12);
	}

	SECTION("even number of values return the larger of the two median values")
	{
		std::vector<int> even = { 42, 5, 12, 93 };
		REQUIRE(emg::Maths::median(even) == 42);
	}
}
