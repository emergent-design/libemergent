#pragma once

#include <string>
#include <string_view>
#include <iomanip>

#ifdef __linux
	#include <unistd.h>
	#include <sys/ioctl.h>
#endif


namespace emergent
{
	// Console helper functions
	struct Console
	{
		// Determine the console width on linux systems.
		static size_t Width()
		{
			#ifdef __linux
				#ifdef TIOCGSIZE
					struct ttysize ts;
					ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
					return ts.ts_cols;
				#elif defined(TIOCGWINSZ)
					struct winsize ts;
					ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
					return ts.ws_col;
				#endif
			#endif

			return 80;
		}


		// Format the text to fit the console width.
		// The first line is printed at the current position assuming it has already been pushed out to the padded position.
		// The padding value is the amount of indentation required for subsequent lines of text.
		// The width is the console or required width.
		struct Format
		{
			std::string_view text;
			size_t padding;
			size_t width;

			Format(std::string_view text, const size_t padding, const size_t width = Width())
				: text(text), padding(padding), width(width) {};
		};


		friend std::ostream &operator << (std::ostream &dst, const Format &f)
		{
			if (f.text.empty())
			{
				return dst;
			}

			const auto remaining	= f.width - f.padding;
			const auto size			= f.text.size();
			size_t pos				= 0;

			dst << std::setfill(' ');

			while (pos < size)
			{
				const auto last = size - pos < remaining
					? std::string::npos
					: f.text.find_last_of(" .,([/-", pos + remaining - 1);

				const auto next = f.text.substr(pos, last == std::string::npos || last < pos
					? remaining
					: last + 1 - pos
				);

				if (pos > 0)
				{
					dst << std::setw(f.padding) << ' ';
				}

				dst << next << '\n';
				pos += next.size();
			}

			return dst;
		}


		// Console colours
		static constexpr const char *Reset		= "\e[0m";
		static constexpr const char *Black		= "\e[30m";
		static constexpr const char *Red		= "\e[31m";
		static constexpr const char *Green		= "\e[32m";
		static constexpr const char *Yellow		= "\e[33m";
		static constexpr const char *Blue		= "\e[34m";
		static constexpr const char *Magenta	= "\e[35m";
		static constexpr const char *Cyan		= "\e[36m";
		static constexpr const char *White		= "\e[37m";
	};
}
