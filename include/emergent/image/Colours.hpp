#pragma once

#include <emergent/Emergent.hpp>
// #include <emergent/image/Depth.hpp>


namespace emergent
{
	// typedef Depth::RGB::Colour<byte> Colour;
	static const byte rgb = 3;

	namespace Greyscale
	{
		typedef std::array<byte, 1> Type;
	}

	namespace RGB
	{
		typedef std::array<byte, rgb> Type;
	}

	/// Colour constants
	namespace Colours
	{
		static RGB::Type Black		= {{ 0, 0, 0 }};
		static RGB::Type White		= {{ 255, 255, 255 }};
		static RGB::Type Grey		= {{ 127, 127, 127 }};

		static RGB::Type Red		= {{ 255, 0, 0 }};
		static RGB::Type Green		= {{ 0, 255, 0 }};
		static RGB::Type Blue		= {{ 0, 0, 255 }};

		static RGB::Type DarkRed	= {{ 127, 0, 0 }};
		static RGB::Type DarkGreen	= {{ 0, 127, 0 }};
		static RGB::Type DarkBlue	= {{ 0, 0, 127 }};

		static RGB::Type Yellow		= {{ 255, 255, 0 }};
		static RGB::Type Magenta	= {{ 255, 0, 255 }};
		static RGB::Type Cyan		= {{ 0, 255, 255 }};

		static std::vector<RGB::Type> Palette = {
			White,		Grey,		Red,	Green,		Blue,		DarkRed,
			DarkGreen,	DarkBlue,	Cyan,	Magenta,	Yellow,		Black
		};
	};
}
