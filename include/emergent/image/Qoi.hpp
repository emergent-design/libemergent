#pragma once
#include <emergent/image/ImageBase.hpp>
// #include <iostream>

namespace emergent::image
{
	class Qoi16;

	// Encode/decode the QOI image format
	// template <typename T>
	class Qoi
	{
		public:


			// HDR alternative???
			// template <typename C> static bool Encode(const ImageBase<T> &src, C &dst)
			template <typename C> static bool Encode(const ImageBase<byte> &src, C &dst)
			{
				static_assert(is_contiguous<C>, "destination must be a contiguous container type");
				static_assert(sizeof(typename C::value_type) == 1, "destination must be a byte buffer");

				const size_t width	= src.Width();
				const size_t height	= src.Height();
				const byte depth	= src.Depth();

				if (width == 0 || height == 0 || depth != 3 || width * height > MAX_PIXELS)
				{
					return false;
				}

				// const size_t maxSize = width * height * (depth + 1) + sizeof(Header) + sizeof(PADDING);
				Header header(width, height, depth, 1); //, sizeof(T));

				dst.resize(width * height * (depth + 1) + sizeof(Header) + sizeof(PADDING));
				std::memcpy(dst.data(), &header, sizeof(Header));

				std::array<Pixel, 64> index;
				Pixel previous, current;

				byte *pd	= dst.data() + sizeof(Header);
				int run		= 0;

				std::array<int, 5> opCount = { 0 };

				for (auto *p : src.Pixels())
				{
					current.rgba.r = p[0];
					current.rgba.g = p[1];
					current.rgba.b = p[2];

					// We're not using alpha channels at all, so skip this
					// if (depth == 4)
					// {
					// 	current.rgba.a = p[3];
					// }

					if (current.v == previous.v)
					{
						if (++run == 62)
						{
							opCount[0]++;
							*pd++	= OP_RUN | (run - 1);
							run		= 0;
						}
					}
					else
					{
						if (run)
						{
							opCount[0]++;
							*pd++	= OP_RUN | (run - 1);
							run		= 0;
						}

						const int lookup = Hash(current);

						if (index[lookup].v == current.v)
						{
							opCount[1]++;
							*pd++ = OP_INDEX | lookup;
						}
						else
						{
							index[lookup] = current;

							const signed char dr	= current.rgba.r - previous.rgba.r;
							const signed char dg	= current.rgba.g - previous.rgba.g;
							const signed char db	= current.rgba.b - previous.rgba.b;
							const signed char dgr	= dr - dg;
							const signed char dgb	= db - dg;

							if (dr > -3 && dr < 2 && dg > -3 && dg < 2 && db > -3 && db < 2)
							{
								opCount[2]++;
								*pd++ = OP_DIFF | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2);
							}
							else if (dgr > -9 && dgr < 8 && dg > -33 && dg < 32 && dgb > -9 && dgb < 8)
							{
								opCount[3]++;
								*pd++ = OP_LUMA | (dg + 32);
								*pd++ = (dgr + 8) << 4 | (dgb + 8);
							}
							else
							{
								opCount[4]++;
								*pd++ = OP_RGB;
								*pd++ = current.rgba.r;
								*pd++ = current.rgba.g;
								*pd++ = current.rgba.b;
							}
						}
					}

					previous = current;
				}

				if (run)
				{
					*pd++ = OP_RUN | (run - 1);
				}

				std::memcpy(pd, PADDING.data(), sizeof(PADDING));
				dst.resize(pd + sizeof(PADDING) - dst.data());

				std::cout
					<< "    run=" << opCount[0]
					<< "  index=" << opCount[1]
					<< "   diff=" << opCount[2]
					<< "   luma=" << opCount[3]
					<< "    rgb=" << opCount[4]
					<< '\n';

				return true;
			}


			// template <typename C> static bool Decode(const C &src, ImageBase<T> &dst)
			template <typename C> static bool Decode(const C &src, ImageBase<byte> &dst)
			{
				static_assert(is_contiguous<C>, "source must be a contiguous container type");
				static_assert(sizeof(typename C::value_type) == 1, "source must be a byte buffer");

				if (src.size() < sizeof(Header) + sizeof(PADDING))
				{
					return false;
				}

				Header header;
				std::memcpy(&header, src.data(), sizeof(Header));

				const size_t width	= be32toh(header.width);
				const size_t height	= be32toh(header.height);
				const byte depth	= header.depth;

				if (width == 0 || height == 0 || depth != 3 || header.typesize != 1 || header.magic != htobe32(MAGIC))
				// if (width == 0 || height == 0 || depth != 3 || header.colourspace > 1 || header.magic != htobe32(MAGIC))
				{
					return false;
				}

				if (width * height >= MAX_PIXELS)
				{
					return false;
				}

				dst.Resize(width, height, depth);

				std::array<Pixel, 64> index;
				Pixel pixel;

				const byte *current	= src.data() + sizeof(Header);
				const byte *end		= src.data() + src.size() - sizeof(PADDING);
				// byte *pd			= dst.Data();
				int run				= 0;


				for (auto *p : dst.Pixels())
				{
					if (run)
					{
						run--;
					}
					else if (current < end)
					{
						const int op = *current++;

						if (op == OP_RGB)
						{
							pixel.rgba.r = *current++;
							pixel.rgba.g = *current++;
							pixel.rgba.b = *current++;
						}
						else if ((op & MASK) == OP_INDEX)
						{
							pixel = index[op];
						}
						else if ((op & MASK) == OP_DIFF)
						{
							pixel.rgba.r += ((op >> 4) & 0x03) - 2;
							pixel.rgba.g += ((op >> 2) & 0x03) - 2;
							pixel.rgba.b += ( op       & 0x03) - 2;
						}
						else if ((op & MASK) == OP_LUMA)
						{
							const int o2 = *current++;
							const int dg = (op & 0x3f) - 32;

							pixel.rgba.r += dg - 8 + ((o2 >> 4) & 0x0f);
							pixel.rgba.g += dg;
							pixel.rgba.b += dg - 8 + ( o2       & 0x0f);
						}
						else if ((op & MASK) == OP_RUN)
						{
							run = op & 0x3f;
						}

						index[Hash(pixel)] = pixel;
					}

					p[0] = pixel.rgba.r;
					p[1] = pixel.rgba.g;
					p[2] = pixel.rgba.b;
					// ignoring alpha here
				}

				return false;
			}


		private:

			struct Header
			{
				uint32_t magic		= htobe32(MAGIC);
				uint32_t width		= 0;
				uint32_t height		= 0;
				byte depth			= 0;
				byte typesize		= 1;	// Making use of this field to store the image typesize
				// byte colourspace	= 0;


				Header() = default;
				// Header(const uint32_t width, const uint32_t height, const byte depth)
				// 	: width(htobe32(width)), height(htobe32(height)), depth(depth) {}
				Header(const uint32_t width, const uint32_t height, const byte depth, const byte typesize)
					: width(htobe32(width)), height(htobe32(height)), depth(depth), typesize(typesize) {}

			} __attribute__((packed));

			union Pixel
			{
				// struct { byte r, g, b, a; } rgba;
				struct { byte r, g, b; } rgba;
				uint32_t v = 0xff000000;
			};

			static inline int Hash(const Pixel &p)
			{
				return (p.rgba.r * 3
					+ p.rgba.g * 5
					+ p.rgba.b * 7
					+ 255 * 11) % 64;
			}

			static constexpr byte OP_INDEX	= 0x00;
			static constexpr byte OP_DIFF	= 0x40;
			static constexpr byte OP_LUMA	= 0x80;
			static constexpr byte OP_RUN	= 0xc0;
			static constexpr byte OP_RGB	= 0xfe;
			static constexpr byte OP_RGBA	= 0xff;
			static constexpr byte MASK		= 0xc0;

			static constexpr uint32_t MAGIC					= 'q' << 24 | 'o' << 16 | 'i' << 8 | 'f';
			static constexpr uint32_t MAX_PIXELS			= 400'000'000;
			static constexpr uint32_t HEADER_SIZE			= 14;
			static constexpr std::array<byte, 8> PADDING	= { 0, 0, 0, 0, 0, 0, 0, 1 };


			friend Qoi16;
	};



	class Qoi16
	{
		public:


			// HDR alternative???
			// template <typename C> static bool Encode(const ImageBase<T> &src, C &dst)
			template <typename C> static bool Encode(const ImageBase<uint16_t> &src, C &dst)
			{
				static_assert(is_contiguous<C>, "destination must be a contiguous container type");
				static_assert(sizeof(typename C::value_type) == 1, "destination must be a byte buffer");

				const size_t width	= src.Width();
				const size_t height	= src.Height();
				const byte depth	= src.Depth();

				if (width == 0 || height == 0 || depth != 3 || width * height > Qoi::MAX_PIXELS)
				{
					return false;
				}

				const size_t maxSize = width * height * (depth + 1) * 2 + sizeof(Qoi::Header) + sizeof(Qoi::PADDING);

				dst.resize(maxSize);

				Qoi::Header header(width, height, depth, 2);

				std::memcpy(dst.data(), &header, sizeof(Qoi::Header));

				std::array<Pixel, LOOKUP_SIZE> index;
				Pixel previous, current;

				byte *pd	= dst.data() + sizeof(Qoi::Header);
				int run		= 0;
				// int lookup	= 0;

				std::array<int, 6> opCount = { 0 };
				std::array<int, LOOKUP_SIZE> runCounts = { 0 };

				for (auto *p : src.Pixels())
				{
					current.rgba.r = p[0];
					current.rgba.g = p[1];
					current.rgba.b = p[2];

				 	if (current.v == previous.v)
				 	{
				 		if (++run == RUN_SIZE)
				 		{
				 			opCount[0]++;
				 			runCounts[run]++;
				 			*pd++	= OP_RUN | (run - 1) >> 8;
				 			*pd++	= (run - 1) & 0xff;
				 			run		= 0;
				 		}
				 	}
				 	else
				 	{
				 		if (run)
				 		{
					 		runCounts[run]++;

				 			if (run > 32)
				 			{
				 				opCount[0]++;
					 			*pd++	= OP_RUN | (run - 1) >> 8;
					 			*pd++	= (run - 1) & 0xff;
					 		}
					 		else
				 			{
				 				opCount[1]++;
				 				*pd++ = OP_RUN5 | (run - 1);
				 			}

					 		run	= 0;
				 		}

						const int lookup = Hash(current);
						// const int found = Find(index, current);

						if (index[lookup].v == current.v)
						// if (found >= 0)
						{
							opCount[2]++;
							*pd++ = OP_INDEX | (lookup) >> 8;
							*pd++ = lookup & 0xff;
							// *pd++ = OP_INDEX | (found) >> 8;
							// *pd++ = found & 0xff;
						}
						else
						{
							index[lookup] = current;
							// lookup			= (lookup + 1) % LOOKUP_SIZE;

							const int dr	= current.rgba.r - previous.rgba.r;
							const int dg	= current.rgba.g - previous.rgba.g;
							const int db	= current.rgba.b - previous.rgba.b;
							const int dgr	= dr - dg;
							const int dgb	= db - dg;

							// if (dr > -9 && dr < 8 && dg > -9 && dg < 8 && db > -9 && db < 9)
							// {
							// 	opCount[3]++;
							// 	*pd++ = OP_DIFF | (dr + 8);
							// 	*pd++ = (dg + 8) << 4 | (db + 8);
							// }
							if (dg > -65 && dg < 64 && dr > -65 && dr < 64 && db > -65 && db < 64)
							{
								opCount[3]++;
								*pd++ = OP_DIFF | ((dr + 64) >> 5);
								*pd++ = ((dr + 64) & 0x03) << 6 | (((dg + 64) & 0x3f) >> 1);
								*pd++ = (((dg + 64) & 0x40) << 1) | (db + 64);
							}
							// else if (dg > -2049 && dg < 2048 && dgr > -129 && dgr < 128 && dgb > -129 && dgb < 128)
							// {
							// 	opCount[4]++;
							// 	*pd++ = OP_LUMA | ((dg + 2048) >> 8);
							// 	*pd++ = (dg + 2048) & 0xff;
							// 	*pd++ = (dgr + 128);
							// 	*pd++ = (dgb + 128);
							// }
							else if (dg > -16385 && dg < 16384 && dgr > -1025 && dgr < 1024 && dgb > -1025 && dgb < 1024)
							{
								opCount[4]++;
								*pd++ = OP_LUMA | ((dg + 16384) >> 10);
								*pd++ = ((dg + 16384) >> 2) & 0xff;
								*pd++ = (((dg + 16384) & 0x03) << 6) | ((dgr + 1024) >> 5);
								*pd++ = (((dgr + 1024) & 0x1f) << 3) | ((dgb + 1024) >> 8);
								*pd++ = (db + 1024) & 0xff;
							}
							else
							{
								opCount[5]++;
								*pd++ = OP_RGB;
								*pd++ = current.rgba.r >> 8;
								*pd++ = current.rgba.r & 0xff;
								*pd++ = current.rgba.g >> 8;
								*pd++ = current.rgba.g & 0xff;
								*pd++ = current.rgba.b >> 8;
								*pd++ = current.rgba.b & 0xff;
							}
						}
					}

					previous = current;
				}

				if (run)
		 		{
		 			if (run > 32)
		 			{
		 				opCount[0]++;
			 			*pd++	= OP_RUN | (run - 1) >> 8;
			 			*pd++	= (run - 1) & 0xff;
			 		}
			 		else
		 			{
		 				opCount[1]++;
		 				*pd++ = OP_RUN5 | (run - 1);
		 			}

		 			// opCount[0]++;
		 			// runCounts[run]++;
		 			// *pd++	= OP_RUN | (run - 1) >> 8;
		 			// *pd++	= (run - 1) & 0xff;
		 			// run		= 0;
		 		}

				std::memcpy(pd, Qoi::PADDING.data(), sizeof(Qoi::PADDING));

				dst.resize(pd + sizeof(Qoi::PADDING) - dst.data());

				// const int total = width * height * depth;
				const int total = std::accumulate(opCount.begin(), opCount.end(), 0);

				std::cout
					<< "  pixels=" << width * height * depth
					<< "  ops=" << total
					<< "  run=" << Print(opCount[0], total)
					<< "  run5=" << Print(opCount[1], total)
					<< "  index=" << Print(opCount[2], total)
					<< "  diff=" << Print(opCount[3], total)
					<< "  luma=" << Print(opCount[4], total)
					<< "  rgb=" << Print(opCount[5], total)
					<< "  ratio=" << ((double)dst.size() / (double)(width * height * depth * 2))
					<< '\n';

				// std::cout << "runs=";
				// for (auto &r : runCounts)
				// {
				// 	std::cout << r << " ";
				// }
				// std::cout << '\n';

				// // std::cout << "actual: " << dst.size() << '\n';

				return true;
			}


			static string Print(const int value, const int total)
			{
				return emg::String::format("%d (%.3f)", value, (double)value / (double)total);
			}



			// template <typename C> static bool Decode(const C &src, ImageBase<T> &dst)
			template <typename C> static bool Decode(const C &src, ImageBase<uint16_t> &dst)
			{
				return false;

				// static_assert(is_contiguous<C>, "source must be a contiguous container type");
				// static_assert(sizeof(typename C::value_type) == 1, "source must be a byte buffer");

				// if (src.size() < sizeof(Header) + sizeof(PADDING))
				// {
				// 	return false;
				// }

				// Header header;
				// std::memcpy(&header, src.data(), sizeof(Header));

				// const size_t width	= be32toh(header.width);
				// const size_t height	= be32toh(header.height);
				// const byte depth	= header.depth;

				// // if (width == 0 || height == 0 || depth != 3 || header.typesize != sizeof(T) || header.magic != htobe32(MAGIC))
				// if (width == 0 || height == 0 || depth != 3 || header.colourspace > 1 || header.magic != htobe32(MAGIC))
				// {
				// 	return false;
				// }

				// if (width * height >= MAX_PIXELS)
				// {
				// 	return false;
				// }

				// dst.Resize(width, height, depth);

				// std::array<Pixel, 64> index;
				// Pixel pixel;

				// const byte *current	= src.data() + sizeof(Header);
				// const byte *end		= src.data() + src.size() - sizeof(PADDING);
				// // byte *pd			= dst.Data();
				// int run				= 0;


				// for (auto *p : dst.Pixels())
				// {
				// 	if (run)
				// 	{
				// 		run--;
				// 	}
				// 	else if (current < end)
				// 	{
				// 		const int op = *current++;

				// 		if (op == OP_RGB)
				// 		{
				// 			// if constexpr (std::is_same_v<T, uint16_t>)
				// 			// {
				// 			// 	pixel.rgba.r = ((uint16_t *)current)[0];
				// 			// 	pixel.rgba.g = ((uint16_t *)current)[1];
				// 			// 	pixel.rgba.b = ((uint16_t *)current)[2];
				// 			// 	current += 6;
				// 			// }
				// 			// else
				// 			// {
				// 				pixel.rgba.r = *current++;
				// 				pixel.rgba.g = *current++;
				// 				pixel.rgba.b = *current++;
				// 			// }
				// 		}
				// 		else if ((op & MASK) == OP_INDEX)
				// 		{
				// 			pixel = index[op];
				// 		}
				// 		else if ((op & MASK) == OP_DIFF)
				// 		{
				// 			pixel.rgba.r += ((op >> 4) & 0x03) - 2;
				// 			pixel.rgba.g += ((op >> 2) & 0x03) - 2;
				// 			pixel.rgba.b += ( op       & 0x03) - 2;
				// 		}
				// 		else if ((op & MASK) == OP_LUMA)
				// 		{
				// 			const int o2 = *current++;
				// 			const int dg = (op & 0x3f) - 32;

				// 			pixel.rgba.r += dg - 8 + ((o2 >> 4) & 0x0f);
				// 			pixel.rgba.g += dg;
				// 			pixel.rgba.b += dg - 8 + ( o2       & 0x0f);
				// 		}
				// 		else if ((op & MASK) == OP_RUN)
				// 		{
				// 			run = op & 0x3f;
				// 		}

				// 		index[Hash(pixel)] = pixel;
				// 	}

				// 	p[0] = pixel.rgba.r;
				// 	p[1] = pixel.rgba.g;
				// 	p[2] = pixel.rgba.b;
				// 	// ignoring alpha here
				// }

				// return false;
			}


		private:

			static constexpr byte OP_INDEX	= 0b0000'0000;
			static constexpr byte OP_DIFF	= 0b0010'0000;
			static constexpr byte OP_LUMA	= 0b0100'0000;
			static constexpr byte OP_RUN	= 0b1000'0000;
			static constexpr byte OP_RUN5	= 0b1010'0000;
			static constexpr byte OP_RGB	= 0b1110'0000;
			// static constexpr byte OP_RGBA	= 0b1111'0000;
			static constexpr byte MASK		= 0b1111'0000;

			// static constexpr size_t LOOKUP_SIZE = 64 * 256 - 1;	// Size of the pixel lookup
			static constexpr size_t LOOKUP_SIZE = 4095;	// Size of the pixel lookup
			// static constexpr size_t LOOKUP_SIZE = 64;	// Size of the pixel lookup
			static constexpr size_t RUN_SIZE	= 4096;	// Size of the run length

			// struct Header
			// {

			// 	// const char magic[4] = { 'q', 'o', 'i', 'f' };
			// 	uint32_t magic		= htobe32(MAGIC);
			// 	uint32_t width		= 0;
			// 	uint32_t height		= 0;
			// 	byte depth			= 0;
			// 	// byte typesize		= 1;	// Making use of this field to store the image typesize
			// 	byte colourspace	= 0;


			// 	Header() = default;
			// 	Header(const uint32_t width, const uint32_t height, const byte depth)
			// 		: width(htobe32(width)), height(htobe32(height)), depth(depth) {}
			// 	// Header(const uint32_t width, const uint32_t height, const byte depth, const byte typesize)
			// 	// 	: width(htobe32(width)), height(htobe32(height)), depth(depth), typesize(typesize) {}

			// } __attribute__((packed));

			union Pixel
			{
				struct { uint16_t r, g, b; } rgba;
				uint64_t v = 0;
			};

			// typedef std::conditional_t<std::is_floating_point_v<T>, double, int64_t> Sum;

			static inline int Hash(const Pixel &p)
			{
				// return (p.rgba.r * 3
				// 	+ p.rgba.g * 5
				// 	+ p.rgba.b * 7
				// ) % LOOKUP_SIZE;

				return (p.rgba.r * 13
					+ p.rgba.g * 37
					+ p.rgba.b * 67
				) % LOOKUP_SIZE;

				// return (p.rgba.r * 3
				// 	+ p.rgba.g * 5
				// 	+ p.rgba.b * 7
				// ) % 64;
			}


			// static inline int Find(const std::array<Pixel, LOOKUP_SIZE> &index, const Pixel &p)
			// {
			// 	for (size_t i=0; i<LOOKUP_SIZE; i++)
			// 	{
			// 		if (p.v == index[i].v)
			// 		{
			// 			return i;
			// 		}
			// 	}

			// 	return -1;
			// }

			// static inline byte *Run(byte *dst, const int value)
			// {
			// 	dst[0]	= OP_RUN | (value >> 8);
			// 	dst[1]	= value & 0xff;
			// 	return dst + 2;
			// }


			// static constexpr size_t RUN_SIZE	= 64;

			// static constexpr uint32_t MAGIC					= 'q' << 24 | 'o' << 16 | 'i' << 8 | 'f';
			// static constexpr uint32_t MAX_PIXELS			= 400'000'000;
			// static constexpr std::array<byte, 8> PADDING	= { 0, 0, 0, 0, 0, 0, 0, 1 };


	};
}
