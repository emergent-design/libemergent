#pragma once

#include <emergent/String.hpp>
#include <string>
#include <string_view>
#include <iomanip>
#include <optional>
// #include <cstring>

#ifdef __linux
	#include <unistd.h>
	#include <sys/ioctl.h>
#endif


// #include <iostream>

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
			auto padding			= 0;

			dst << std::setfill(' ');

			for (auto line : String::explode(f.text, "\n"))
			{
				line = String::trim(line, " \t");

				while (line.length() >= remaining)
				{
					const auto split = std::min(
						line.find_last_of(" .,(/-", remaining - 1),
						remaining
					);

					dst << std::setw(padding) << "" << line.substr(0, split) << '\n';

					line	= line.substr(split + 1);
					padding = f.padding;
				}

				dst << std::setw(padding) << "" << line <<'\n';
				padding = f.padding;
			}

			return dst;
		}


		// Console colours
		static constexpr const char *Reset		= "\x1B[0m";
		static constexpr const char *Black		= "\x1B[30m";
		static constexpr const char *Red		= "\x1B[31m";
		static constexpr const char *Green		= "\x1B[32m";
		static constexpr const char *Yellow		= "\x1B[33m";
		static constexpr const char *Blue		= "\x1B[34m";
		static constexpr const char *Magenta	= "\x1B[35m";
		static constexpr const char *Cyan		= "\x1B[36m";
		static constexpr const char *White		= "\x1B[37m";
		static constexpr const char *Default	= "\x1B[39m";

		static constexpr const char *BrightYellow = "\x1B[93m";

		// static constexpr const size_t ColourSize = std::char_traits<char>::length(Red);

		// Console commands
		static constexpr const char *Erase		= "\x1B[2K\r";
	};


	namespace console
	{
		// Draw a hozizontal line of a given length
		struct Line { size_t length = 0; };

		inline std::ostream &operator <<(std::ostream &dst, const Line &l)
		{
			for (size_t i=0; i<l.length; i++)
			{
				dst << "\u2500";
			}

			return dst;
		}

		// struct Centre { std::string_view item; size_t width; };

		// inline std::ostream &operator <<(std::ostream &dst, const Centre &c)
		// {
		// 	if (c.width > c.item.length())
		// 	{
		// 		const size_t space = c.width - c.item.length();

		// 		dst << std::setw(space - space / 2) << ""
		// 			<< c.item
		// 			<< std::setw(space / 2) << "";
		// 	}
		// 	else
		// 	{
		// 		dst << c.item.substr(0, c.width);
		// 	}

		// 	return dst;
		// }


		// Return the size of the largest item in the values container
		template <typename T> inline size_t MaxItemSize(const T &values)
		{
			if (values.empty())
			{
				return 0;
			}

			#if defined(__cpp_lib_ranges)
				return std::ranges::max(values, {}, &T::value_type::size).size();
			#else
				return std::max_element(
					values.begin(), values.end(),
					[](auto a, auto b) { return a.size() < b.size(); }
				)->size();
			#endif
		}


		// Helper class for printing a table of data.
		// Column and row headers can be defined where required.
		// Each cell can contain multiple strings - placed on separate lines.
		// Column width can be fixed or calculated automatically.
		template <size_t Columns> struct Table
		{
			struct Row
			{
				std::string header;
				std::array<std::vector<std::string>, Columns> cells;


				Row() = default;
				Row(const std::string &header) : header(header) {}

				size_t Height() const
				{
					// Not actually widest item here, but largest vector in the cells array
					return MaxItemSize(cells);
				}
			};

			std::array<std::string, Columns> headers;
			std::vector<Row> rows;
			size_t width	= 0;	// fixed column width, leave as 0 for auto column sizing
			size_t padding	= 1;	// horizontal cell padding

			Table &Width(size_t value)
			{
				this->width = value;
				return *this;
			}

			Table &Padding(size_t value)
			{
				this->padding = value;
				return *this;
			}

			Table &Headers(const std::array<std::string, Columns> &values)
			{
				this->headers = values;
				return *this;
			}

			Row &AddRow(const std::string &header = "")
			{
				return this->rows.emplace_back(header);
			}


			size_t Widest(const size_t column) const
			{
				if (this->width)
				{
					return this->width;
				}

				size_t max = this->headers[column].size();

				for (auto &r : this->rows)
				{
					max = std::max(max, MaxItemSize(r.cells[column]));
				}

				return max;
			}


			std::array<size_t, Columns + 1> ColumnWidths() const
			{
				std::array<size_t, Columns + 1> widths	= { 0 };

				// find the maximum row header width
				for (auto &r : this->rows)
				{
					widths[0] = std::max(widths[0], r.header.size());
				}

				// find the maximum string width for each column
				for (size_t i=0; i<Columns; i++)
				{
					widths[i+1] = this->Widest(i);
				}

				return widths;
			}


			void ColumnHeaders(std::ostream &dst, const std::array<size_t, Columns + 1> &widths, const bool rowHeader) const
			{
				if (MaxItemSize(this->headers) == 0)
				{
					// There are no column headers, so return
					return;
				}

				dst << Console::Blue << "\u2502";

				if (rowHeader)
				{
					 dst << std::setw(widths[0] + 2 * this->padding) << "" << "\u2502";
				}

				for (size_t i=0; i<Columns; i++)
				{
					// dst << std::setw(this->padding) << ""
					// 	<< Centre { this->headers[i], widths[i+1] }
					// 	<< std::setw(this->padding) << "";

					dst << std::setw(widths[i + 1] + this->padding)
						<< this->headers[i]
						<< std::setw(this->padding)
						<< "";
				}

				dst << "\u2502\n";
			}


			void RowHeader(std::ostream &dst, const std::array<size_t, Columns + 1> &widths, const bool first, std::optional<std::string_view> header) const
			{
				dst << Console::Blue << "\u2502";

				if (header)
				{
					dst << std::setw(widths[0] + this->padding)
						<< (first ? header.value() : "")
						<< std::setw(this->padding) << "" << "\u2502";
				}

				dst << Console::Reset;
			}


			static inline std::string_view RowItem(std::string_view item, const size_t width)
			{
				// truncate the string for when using fixed with columns
				return item.substr(0, width);
			}


			void RowMain(std::ostream &dst, const std::array<size_t, Columns + 1> &widths, const Row &r, const size_t item) const
			{
				static constexpr std::array PALLETTE = {
					Console::Reset, Console::Yellow, Console::Cyan, Console::Magenta
				};

				for (size_t i=0; i<Columns; i++)
				{
					auto &cell = r.cells[i];

					if (item < cell.size())
					{
						dst << PALLETTE[item % sizeof(PALLETTE)]
							<< std::setw(widths[i+1] + this->padding)
							<< RowItem(cell[item], widths[i+1])
							<< std::setw(this->padding) << "";
					}
					else
					{
						// you could potentially have different amounts of data
						// in each cell - so leave blank entries for anything missing
						dst << std::setw(widths[i+1] + 2 * this->padding) << "";
					}
				}

				dst << Console::Blue << "\u2502\n";
			}


			std::ostream &Render(std::ostream &dst) const
			{
				const auto widths		= this->ColumnWidths();
				const size_t bothPads	= this->padding * 2;
				const size_t total		= std::accumulate(widths.begin(), widths.end(), widths.size() * bothPads);
				const size_t header		= widths[0] ? widths[0] + bothPads : 0;
				const size_t main		= total - (header ? header : bothPads);

				Table<Columns>::Separator(dst, header, main, "\u250c", "\u252c", "\u2510");

				this->ColumnHeaders(dst, widths, header);

				for (auto &r : this->rows)
				{
					Table<Columns>::Separator(dst, header, main, "\u251c", "\u253c", "\u2524");

					const size_t height = r.Height();

					for (size_t y=0; y<height; y++)
					{
						this->RowHeader(dst, widths, y == 0, header
							? std::make_optional(r.header)
							: std::nullopt
						);

						this->RowMain(dst, widths, r, y);
					}
				}

				Table<Columns>::Separator(dst, header, main, "\u2514", "\u2534", "\u2518");

				return dst;
			}


			// Draw a table row separator - the left/middle/right indicate which unicode box drawing symbols
			// to use so that it can be customised to also print the top and bottom of the table
			static void Separator(std::ostream &dst, const size_t header, const size_t main, std::string_view left, std::string_view middle, std::string_view right)
			{
				dst << Console::Blue << left;

				if (header)
				{
					dst << Line { .length = header } << middle;
				}

				dst << Line { .length = main } << right << '\n' << Console::Reset;
			}


			// Produce a table from a vector of values. The column headers will be the column index and the row headers will be the
			// offset in the vector. The formatters will convert a value for a specific cell, multiple formatters will result in multiple
			// representations within a cell drawn on different lines.
			// For example:
			//
			//   std::cout << Table<10>::From(data, { "%d", "0x%04x" });
			//
			// which given a vector of uint16_t will print the decimal and hex values in each cell as follows
			// ┌────┬────────────────────────────────────────────────────────────────────────────────┐
			// │    │      0       1       2       3       4       5       6       7       8       9 │
			// ├────┼────────────────────────────────────────────────────────────────────────────────┤
			// │  0 │      1       2       3    4095      42    8192      12     435     223       1 │
			// │    │ 0x0001  0x0002  0x0003  0x0fff  0x002a  0x2000  0x000c  0x01b3  0x00df  0x0001 │
			// ├────┼────────────────────────────────────────────────────────────────────────────────┤
			// │ 10 │      2       3    4095      42    8192      12     435     223       1       2 │
			// │    │ 0x0002  0x0003  0x0fff  0x002a  0x2000  0x000c  0x01b3  0x00df  0x0001  0x0002 │
			//                                          ...
			template <typename T> static Table From(const T &values, std::initializer_list<const char *> formatters)
			{
				const size_t size = values.size();

				Table table;

				for (size_t i=0; i<Columns; i++)
				{
					table.headers[i] = std::to_string(i);
				}

				for (size_t offset=0; offset<size; offset+=Columns)
				{
					Row row { std::to_string(offset) };

					const size_t remaining	= std::min(Columns, size - offset);

					for (size_t i=0; i<remaining; i++)
					{
						for (auto &f : formatters)
						{
							row.cells[i].push_back(String::format(f, values[offset + i]));
						}
					}

					table.rows.push_back(row);
				}

				return table;
			}
		};


		template <size_t Columns> std::ostream &operator <<(std::ostream &dst, const Table<Columns> &table)
		{
			return table.Render(dst);
		}
	}

}
