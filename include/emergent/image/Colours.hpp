#pragma once

#include <emergent/Emergent.hpp>


namespace emergent
{
	// typedef Depth::RGB::Colour<byte> Colour;
	static const byte rgb = 3;

	namespace Greyscale
	{
		typedef std::array<byte, 1> Type;

		static const Type Black = {{ 0 }};
		static const Type Grey	= {{ 128 }};
		static const Type White = {{ 255 }};

		static const std::vector<Type> Palette {
			Black, Grey, White
		};
	}

	namespace RGB
	{
		typedef std::array<byte, rgb> Type;

		static const Type Black		= {{ 0, 0, 0 }};
		static const Type White		= {{ 255, 255, 255 }};
		static const Type Grey		= {{ 127, 127, 127 }};

		static const Type Red		= {{ 255, 0, 0 }};
		static const Type Green		= {{ 0, 255, 0 }};
		static const Type Blue		= {{ 0, 0, 255 }};

		static const Type DarkRed	= {{ 127, 0, 0 }};
		static const Type DarkGreen	= {{ 0, 127, 0 }};
		static const Type DarkBlue	= {{ 0, 0, 127 }};

		static const Type Yellow	= {{ 255, 255, 0 }};
		static const Type Magenta	= {{ 255, 0, 255 }};
		static const Type Cyan		= {{ 0, 255, 255 }};

		static const std::vector<Type> Palette = {
			White,		Grey,		Red,	Green,		Blue,		DarkRed,
			DarkGreen,	DarkBlue,	Cyan,	Magenta,	Yellow,		Black
		};
	};
}
