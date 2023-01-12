#include "doctest.h"
#include <emergent/image/Buffer.hpp>

using emg::image::Buffer;
using emg::byte;


TEST_SUITE("buffer")
{
	TEST_CASE("constructing an empty buffer")
	{

		Buffer<byte> buffer;

		SUBCASE("size is zero")					CHECK(buffer.size() == 0);
		SUBCASE("empty is true")				CHECK(buffer.empty());
		SUBCASE("begin and end are the same")	CHECK(buffer.begin() == buffer.end());
	}


	TEST_CASE("constructing a buffer with initial size")
	{
		Buffer<byte> buffer(8);

		SUBCASE("size is expected")					CHECK(buffer.size() == 8);
		SUBCASE("memory has been allocated")		CHECK(buffer.data() != nullptr);
		SUBCASE("begin and end are not the same")	CHECK(buffer.begin() != buffer.end());
	}


	TEST_CASE("resizing")
	{
		Buffer<byte> buffer;
		buffer.resize(8);

		SUBCASE("resize allocates memory")
		{
			CHECK(buffer.size() == 8);
			CHECK(buffer.data() != nullptr);
		}

		SUBCASE("resizing down truncates but does not reallocate memory")
		{
			const auto ptr = buffer.data();

			buffer.resize(0);
			CHECK(buffer.empty());
			CHECK(buffer.data() == ptr);
		}
	}


	TEST_CASE_TEMPLATE("comparison", T, byte, int, double)
	{
		Buffer<T> buffer(8);
		std::fill(buffer.begin(), buffer.end(), 42);

		SUBCASE("buffers of different size are not equal")
		{
			Buffer<T> other(4);
			std::fill(other.begin(), other.end(), 42);

			CHECK(other != buffer);
		}

		SUBCASE("buffers with differing values are not equal")
		{
			Buffer<T> other(8);
			std::fill(other.begin(), other.end(), 2);

			CHECK(other != buffer);
			CHECK_FALSE(other == buffer);
		}

		SUBCASE("buffers of the same size and values are equal")
		{
			Buffer<T> other(8);
			std::fill(other.begin(), other.end(), 42);

			CHECK(other == buffer);
		}
	}


	TEST_CASE("copying")
	{
		Buffer<byte> buffer(8);
		std::fill(buffer.begin(), buffer.end(), 42);

		CHECK(buffer.data()[4] == 42);

		SUBCASE("constructing a buffer from another allocates new memory")
		{
			Buffer<byte> other = buffer;

			CHECK(other.size() == 8);
			CHECK(other.data() != buffer.data());
			CHECK(other == buffer);
		}

		SUBCASE("move constructing a buffer changes ownership of the storage")
		{
			const auto ptr		= buffer.data();
			Buffer<byte> other	= std::move(buffer);

			CHECK(other.size() == 8);
			CHECK(other.data() == ptr);
			CHECK(buffer.data() == nullptr);
		}

		SUBCASE("copying a buffer allocates new memory")
		{
			Buffer<byte> other;
			other = buffer;

			CHECK(other.size() == 8);
			CHECK(other.data() != buffer.data());
			CHECK(other == buffer);
		}
	}


	TEST_CASE_TEMPLATE("pointers can be used to access the raw data", T, emg::byte, int, double)
	{
		Buffer<T> buffer(8);

		auto ptr = buffer.data();

		for (size_t i=0; i<buffer.size(); i++)
		{
			ptr[i] = 42;
		}

		CHECK(std::all_of(buffer.begin(), buffer.end(), [](auto b) {
			return b == 42;
		}));
	}


	TEST_CASE("data cab be accessed using array operators")
	{
		Buffer<byte> buffer(8);

		for (size_t i=0; i<buffer.size(); i++)
		{
			buffer[i] = 42;
		}

		CHECK(std::all_of(buffer.begin(), buffer.end(), [](auto b) {
			return b == 42;
		}));
	}


	TEST_CASE("iterating")
	{
		SUBCASE("can iterate over and modify a non-const buffer")
		{
			Buffer<byte> buffer(8);

			for (auto &b : buffer)
			{
				b = 42;
			}

			CHECK(std::all_of(buffer.begin(), buffer.end(), [](auto b) {
				return b == 42;
			}));
		}

		SUBCASE("can iterator over a const buffer")
		{
			Buffer<byte> buffer(8);
			std::fill(buffer.begin(), buffer.end(), 42);

			CHECK(std::all_of(buffer.cbegin(), buffer.cend(), [](auto b) {
				return b == 42;
			}));

			const Buffer<byte> const_buffer = buffer;

			CHECK(std::all_of(const_buffer.begin(), const_buffer.end(), [](auto b) {
				return b == 42;
			}));
		}
	}
}
