#pragma once

#include <emergent/Emergent.h>
#include <emergent/image/Depth.h>


namespace emergent
{
	typedef Depth::RGB::Colour<byte> Colour;


	/// Colour constants
	namespace Colours
	{
		static Colour Black(0, 0, 0);
		static Colour White(255, 255, 255);
		static Colour Grey(127, 127, 127);

		static Colour Red(255, 0, 0);
		static Colour Green(0, 255, 0);
		static Colour Blue(0, 0, 255);

		static Colour DarkRed(127, 0, 0);
		static Colour DarkGreen(0, 127, 0);
		static Colour DarkBlue(0, 0, 127);

		static Colour Yellow(255, 255, 0);
		static Colour Magenta(255, 0, 255);
		static Colour Cyan(0, 255, 255);

		static std::vector<Colour> Palette {
			Colours::White,		Colours::Grey,		Colours::Red,
			Colours::Green,		Colours::Blue,		Colours::DarkRed,
			Colours::DarkGreen, Colours::DarkBlue,	Colours::Cyan,
			Colours::Magenta,	Colours::Yellow
		};
	};
}
