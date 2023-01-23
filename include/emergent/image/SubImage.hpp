#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/image/Iterator.hpp>


// namespace emergent::image  // >= c++17 only :(
namespace emergent { namespace image
{
	template <class T> struct SubImage
	{
		T *data				= nullptr;	// Points to the top-left corner of the sub-image in the original source image data
		const byte depth	= 1;		// Depth of the original source image data
		const size_t width	= 0;		// Width of the sub-image
		const size_t height	= 0;		// Height of the sub-image
		const size_t row	= 0;		// Row size of the original source image data


		operator bool() const { return data; }


		// Return an iterator to a row in the sub-image. It will automatically step between pixels
		// and provide a pointer to the current position which gives access to all channels.
		image::Iterator<T> Row(const int y)
		{
			return y >= 0 && y < height
				? image::Iterator<T>(this->data + y * this->row, this->width, this->depth)
				: image::Iterator<T>();
		}

		// Return a const iterator to a row in the sub-image. It will automatically step between pixels
		// and provide a pointer to the current position which gives access to all channels.
		image::Iterator<const T> Row(const int y) const
		{
			return y >= 0 && y < this->height
				? image::Iterator<const T>(this->data + y * this->row, this->width, this->depth)
				: image::Iterator<const T>();
		}

		// Return an iterator to a column in the sub-image. It will automatically step between rows
		// and provide a pointer to the current position which gives access to all channels.
		image::Iterator<T> Columns(const int x)
		{
			return x >= 0 && x < this->width
				? image::Iterator<T>(this->data + x * this->depth, this->height, this->row)
				: image::Iterator<T>();
		}

		// Return a const iterator to a column in the sub-image. It will automatically step between rows
		// and provide a pointer to the current position which gives access to all channels.
		image::Iterator<const T> Columns(const int x) const
		{
			return x >= 0 && x < this->width
				? image::Iterator<const T>(this->data + x * this->depth, this->height, this->row)
				: image::Iterator<const T>();
		}
	};
}}
