#pragma once
#include <cstddef>
#include <cstdint>

namespace emergent
{
	// T is the underlying storage size of the bitflags
	template <size_t Index, typename T = uint64_t> class Bit
	{
		static_assert(Index < 8 * sizeof(T), "Cannot index a bit beyond the size of the underlying storage");

		public:

			Bit &operator=(bool value)
			{
				this->value = (this->value & ~(T(1u) << Index)) | (T(value) << Index);
				return *this;
			}

			operator bool() const
			{
				return this->value & (T(1u) << Index);
			}

		private:

			T value;
	};


	template <size_t Index, size_t Bits, typename T = uint64_t> class BitField
	{
		static_assert(Index + Bits <= 8 * sizeof(T), "A bitfield cannot exceed the bounds of the underlying storage");

		public:

			BitField &operator=(T value)
			{
				this->value = (this->value & ~(MASK << Index)) | ((value & MASK) << Index);
				return *this;
			}

			operator T() const		{ return (this->value >> Index) & MASK; }
			BitField &operator++()	{ return *this = *this + 1; }
			BitField &operator--()	{ return *this = *this - 1; }
			// T operator++(int)		{ T r = *this; ++*this; return r; }
			// T operator--(int)		{ T r = *this; --*this; return r; }

		private:

			static const T MASK = (T(1u) << Bits) - T(1u);
			T value;
	};
}

