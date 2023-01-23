#pragma once

#include <emergent/Emergent.hpp>
#include <cstring>


// namespace emergent::image  // >= c++17 only :(
namespace emergent { namespace image
{
	// A simple buffer for ImageBase<> to use instead of std::vector<>, but since images are
	// only ever dealing with fundamental types then there is no need to invoke destructors
	// and so this is much more efficient than vector for resizing.
	// It meets some of the requirements for a Container type.
	template <typename T> struct Buffer
	{
		using value_type		= T;
		using size_type			= size_t;
		using iterator 			= typename std::vector<T>::iterator;
		using const_iterator	= typename std::vector<T>::const_iterator;

		T *storage		= nullptr;
		size_t used		= 0;
		size_t capacity = 0;


		Buffer() = default;

		Buffer(const Buffer<T> &other)
		{
			*this = other;
		}

		Buffer(Buffer<T> &&other) : storage(other.storage), used(other.used), capacity(other.capacity)
		{
			other.storage	= nullptr;
			other.used		= other.capacity = 0;
		}

		explicit Buffer(const size_t size)
		{
			this->resize(size);
		}

		~Buffer()
		{
			if (this->storage)
			{
				delete [] this->storage;
			}
		}


		Buffer& operator=(const Buffer &other)
		{
			this->resize(other.size());
			std::memcpy(this->storage, other.storage, other.size() * sizeof(T));
			return *this;
		}


		inline bool operator==(const Buffer &other) const
		{
			return this->size() == other.size()
				&& std::equal(this->begin(), this->end(), other.begin());
		}

		inline bool operator!=(const Buffer &other) const
		{
			return !(*this == other);
		}


		void resize(const size_t size)
		{
			if (size > this->capacity)
			{
				if (this->storage)
				{
					delete [] this->storage;
				}

				this->storage	= new T[size];
				this->capacity	= size;
			}

			this->used = size;
		}


		[[nodiscard]] size_t size() const	{ return this->used; }
		[[nodiscard]] bool empty() const	{ return this->used == 0; }
		[[nodiscard]] T *data()				{ return this->storage; }
		[[nodiscard]] const T *data() const	{ return this->storage; }

		[[nodiscard]] T &operator[](size_t n)				{ return this->storage[n]; }
		[[nodiscard]] const T &operator[](size_t n) const	{ return this->storage[n]; }

		[[nodiscard]] iterator begin()				{ return iterator(this->storage); }
		[[nodiscard]] iterator end() 				{ return iterator(this->storage + this->used); }
		[[nodiscard]] const_iterator begin() const	{ return const_iterator(this->storage); }
		[[nodiscard]] const_iterator end() const	{ return const_iterator(this->storage + this->used); }
		[[nodiscard]] const_iterator cbegin() const	{ return const_iterator(this->storage); }
		[[nodiscard]] const_iterator cend() const	{ return const_iterator(this->storage + this->used); }
	};
}}
