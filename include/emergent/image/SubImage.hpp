#pragma once

#include <emergent/Emergent.hpp>


namespace emergent::image
{
	template <class T = byte> struct SubImage
	{
		T *data			= nullptr;	// Points to the top-left corner of the sub-image in the original source image data
		byte depth		= 1;		// Depth of the original source image data
		size_t width	= 0;		// Width of the sub-image
		size_t height	= 0;		// Height of the sub-image
		size_t row		= 0;		// Row size of the original source image data


		operator bool() const { return data; }
	};
}
