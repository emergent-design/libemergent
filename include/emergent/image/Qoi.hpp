#pragma once
#include <emergent/image/ImageBase.hpp>

#if __has_include(<zstd.h>)
	#include <zstd.h>
#endif
#include <iostream>

namespace emergent::image
{
	// Encode/decode the QOI image format. This implementation ignores alpha because ImageBase<> tends
	// to only be greyscale or RGB. It encodes into the supplied buffer and decodes into the supplied
	// image which promotes reuse of memory - useful when handling continuous streams of images.
	// In a change from the QOI standard, the last byte in the header is used to indicate the bit
	// depth of the image (typesize) instead of the colourspace.
	//
	// 16-bit images:
	// Qoi with residuals. Use the standard Qoi method for the upper byte of each channel and then
	// encode the lower bytes (residuals) directly. This poses an issue with the run length encoding
	// because decoding will require the opcode first - so cache the residuals and then push them
	// sequentially into the destination after the run op.
	// This algorithm is based on the assumption that HDR images contain more noise in the lower
	// bits which makes this kind of encoding more difficult, so instead concentrate on shrinking
	// the most significant bits instead.
	class Qoi
	{
		public:

			template <typename T, typename C> static bool Encode(const ImageBase<T> &src, C &dst)
			{
				static_assert(is_contiguous<C>, "destination must be a contiguous container type");
				static_assert(sizeof(typename C::value_type) == 1, "destination must be a byte buffer");
				static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>, "image type must be uint8_t or uint16_t");

				const size_t width	= src.Width();
				const size_t height	= src.Height();
				const byte depth	= src.Depth();

				if (width == 0 || height == 0 || depth != 3 || width * height > MAX_PIXELS)
				{
					return false;
				}

				Header header(width, height, depth, sizeof(T));

				dst.resize(width * height * (depth + 1) * sizeof(T) + sizeof(Header) + sizeof(PADDING));
				std::memcpy(dst.data(), &header, sizeof(Header));

				std::array<Pixel, LOOKUP_SIZE> index = {{}};
				std::array<Pixel, RUN_SIZE+1> residuals;
				Pixel previous, current;

				byte *pd	= dst.data() + sizeof(Header);
				int run		= 0;

				// Write a run length + residuals block to the buffer when dealing with 16-bit images
				auto Run = [](byte *dst, const int run, const std::array<Pixel, RUN_SIZE+1> &residuals) {
					*dst++	= OP_RUN | (run - 1);

					if constexpr (sizeof(T) == 2)
					{
						for (int i=0; i<run; i++)
						{
							*dst++ = residuals[i].rgb.r;
							*dst++ = residuals[i].rgb.g;
							*dst++ = residuals[i].rgb.b;
						}
					}

					return dst;
				};

				for (auto *p : src.Pixels())
				{
					if constexpr (sizeof(T) == 1)
					{
						current.rgb.r = p[0];
						current.rgb.g = p[1];
						current.rgb.b = p[2];

						// We're not using alpha channels at all, so skip this
						// if (depth == 4)
						// {
						// 	current.rgb.a = p[3];
						// }
					}
					else
					{
						current.rgb.r			= p[0] >> 8;
						current.rgb.g			= p[1] >> 8;
						current.rgb.b			= p[2] >> 8;
						residuals[run].rgb.r	= p[0] & 0xff;
						residuals[run].rgb.g	= p[1] & 0xff;
						residuals[run].rgb.b	= p[2] & 0xff;
					}

					if (current.v == previous.v)
					{
						if (++run == RUN_SIZE)
						{
							pd	= Run(pd, run, residuals);
							run	= 0;
						}
					}
					else
					{
						const auto &res	= residuals[run];

						if (run)
						{
							pd	= Run(pd, run, residuals);
							run	= 0;
						}

						const int lookup = Hash(current);

						if (index[lookup].v == current.v)
						{
							*pd++ = OP_INDEX | lookup;
						}
						else
						{
							index[lookup] = current;

							const signed char dr	= current.rgb.r - previous.rgb.r;
							const signed char dg	= current.rgb.g - previous.rgb.g;
							const signed char db	= current.rgb.b - previous.rgb.b;
							const signed char dgr	= dr - dg;
							const signed char dgb	= db - dg;

							if (dr > -3 && dr < 2 && dg > -3 && dg < 2 && db > -3 && db < 2)
							{
								*pd++ = OP_DIFF | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2);
							}
							else if (dgr > -9 && dgr < 8 && dg > -33 && dg < 32 && dgb > -9 && dgb < 8)
							{
								*pd++ = OP_LUMA | (dg + 32);
								*pd++ = (dgr + 8) << 4 | (dgb + 8);
							}
							else
							{
								*pd++ = OP_RGB;
								*pd++ = current.rgb.r;
								*pd++ = current.rgb.g;
								*pd++ = current.rgb.b;
							}
						}

						if constexpr (sizeof(T) == 2)
						{
							*pd++ = res.rgb.r;
							*pd++ = res.rgb.g;
							*pd++ = res.rgb.b;
						}
					}

					previous = current;
				}

				if (run)
				{
					pd = Run(pd, run, residuals);
				}

				std::memcpy(pd, PADDING.data(), sizeof(PADDING));
				dst.resize(pd + sizeof(PADDING) - dst.data());

				return true;
			}


			template <typename T, typename C> static bool Decode(const C &src, ImageBase<T> &dst)
			{
				static_assert(is_contiguous<C>, "source must be a contiguous container type");
				static_assert(sizeof(typename C::value_type) == 1, "source must be a byte buffer");
				static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>, "image type must be uint8_t or uint16_t");

				if (src.size() < sizeof(Header) + sizeof(PADDING))
				{
					return false;
				}

				Header header;
				std::memcpy(&header, src.data(), sizeof(Header));

				const size_t width	= be32toh(header.width);
				const size_t height	= be32toh(header.height);
				const byte depth	= header.depth;

				if (width == 0 || height == 0 || depth != 3 || header.typesize != sizeof(T) || header.magic != htobe32(MAGIC))
				{
					return false;
				}

				if (width * height >= MAX_PIXELS)
				{
					return false;
				}

				dst.Resize(width, height, depth);

				std::array<Pixel, LOOKUP_SIZE> index = {{}};
				Pixel pixel;

				const byte *current	= src.data() + sizeof(Header);
				const byte *end		= src.data() + src.size() - sizeof(PADDING);
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
							pixel.rgb.r = *current++;
							pixel.rgb.g = *current++;
							pixel.rgb.b = *current++;
						}
						else if ((op & MASK) == OP_INDEX)
						{
							pixel = index[op];
						}
						else if ((op & MASK) == OP_DIFF)
						{
							pixel.rgb.r += ((op >> 4) & 0x03) - 2;
							pixel.rgb.g += ((op >> 2) & 0x03) - 2;
							pixel.rgb.b += ( op       & 0x03) - 2;
						}
						else if ((op & MASK) == OP_LUMA)
						{
							const int o2 = *current++;
							const int dg = (op & 0x3f) - 32;

							pixel.rgb.r += dg - 8 + ((o2 >> 4) & 0x0f);
							pixel.rgb.g += dg;
							pixel.rgb.b += dg - 8 + ( o2       & 0x0f);
						}
						else if ((op & MASK) == OP_RUN)
						{
							run = op & 0x3f;
						}

						index[Hash(pixel)] = pixel;
					}

					if constexpr (sizeof(T) == 1)
					{
						p[0] = pixel.rgb.r;
						p[1] = pixel.rgb.g;
						p[2] = pixel.rgb.b;
						// ignoring alpha here
					}
					else
					{
						// Merge the encoded upper bytes with the residuals
						p[0] = (pixel.rgb.r << 8) | *current++;
						p[1] = (pixel.rgb.g << 8) | *current++;
						p[2] = (pixel.rgb.b << 8) | *current++;
						// ignoring alpha here
					}
				}

				return true;
			}


		private:

			static constexpr byte OP_INDEX	= 0x00;
			static constexpr byte OP_DIFF	= 0x40;
			static constexpr byte OP_LUMA	= 0x80;
			static constexpr byte OP_RUN	= 0xc0;
			static constexpr byte OP_RGB	= 0xfe;
			// static constexpr byte OP_RGBA	= 0xff;
			static constexpr byte MASK		= 0xc0;

			static constexpr uint32_t MAGIC					= 'q' << 24 | 'o' << 16 | 'i' << 8 | 'f';
			static constexpr uint32_t MAX_PIXELS			= 400'000'000;
			static constexpr uint32_t HEADER_SIZE			= 14;
			static constexpr size_t LOOKUP_SIZE				= 64;	// Size of the pixel lookup
			static constexpr size_t RUN_SIZE				= 62;	// Size of the run length
			static constexpr std::array<byte, 8> PADDING	= { 0, 0, 0, 0, 0, 0, 0, 1 };

			struct Header
			{
				uint32_t magic		= htobe32(MAGIC);
				uint32_t width		= 0;
				uint32_t height		= 0;
				byte depth			= 0;
				byte typesize		= 1;	// Making use of this field to store the image typesize


				Header() = default;
				Header(const uint32_t width, const uint32_t height, const byte depth, const byte typesize)
					: width(htobe32(width)), height(htobe32(height)), depth(depth), typesize(typesize) {}

			} __attribute__((packed));

			union Pixel
			{
				struct { byte r, g, b; } rgb;
				uint32_t v = 0;
			};

			static inline int Hash(const Pixel &p)
			{
				return (p.rgb.r * 3
					+ p.rgb.g * 5
					+ p.rgb.b * 7
					+ 255 * 11) % 64;
			}

			// // Write a run length + residuals block to the buffer
			// static byte *Run(byte *dst, const int run, const std::array<Pixel, RUN_SIZE+1> &residuals)
			// {
			// 	*dst++	= OP_RUN | (run - 1);

			// 	for (int i=0; i<run; i++)
			// 	{
			// 		*dst++ = residuals[i].rgb.r;
			// 		*dst++ = residuals[i].rgb.g;
			// 		*dst++ = residuals[i].rgb.b;
			// 	}

			// 	return dst;
			// }
	};


#if __has_include(<zstd.h>)

	// Uses Qoi 8/16 bit compression from above but adds zstd compression. This often seems to beat PNG in
	// compression ratio while still being considerably faster overall.
	class Qoiz
	{
		public:

			template <typename T, typename C> static bool Encode(const ImageBase<T> &src, C &dst, C &scratch, const int compression = 1)
			{
				static_assert(is_contiguous<C>, "source must be a contiguous container type");
				static_assert(sizeof(typename C::value_type) == 1, "source must be a byte buffer");

				thread_local auto context = std::unique_ptr<ZSTD_CCtx, void(*)(ZSTD_CCtx*)>(
					ZSTD_createCCtx(),
					[](auto *c) { ZSTD_freeCCtx(c); }
				);

				if (!Qoi::Encode(src, scratch))
				{
					return false;
				}

				const auto capacity = ZSTD_compressBound(scratch.size());

				dst.resize(capacity);

				const auto length = ZSTD_compressCCtx(context.get(), dst.data(), capacity, scratch.data(), scratch.size(), compression);

				if (ZSTD_isError(length))
				{
					return false;
				}

				dst.resize(length);

				return true;
			}


			template <typename T, typename C> static bool Decode(const C &src, ImageBase<T> &dst, C &scratch)
			{
				thread_local auto context = std::unique_ptr<ZSTD_DCtx, void(*)(ZSTD_DCtx*)>(
					ZSTD_createDCtx(),
					[](auto *c) { ZSTD_freeDCtx(c); }
				);

				const auto length = ZSTD_getFrameContentSize(src.data(), src.size());

				if (ZSTD_isError(length))
				{
					return false;
				}

				scratch.resize(length);

				const auto result = ZSTD_decompressDCtx(context.get(), scratch.data(), scratch.size(), src.data(), src.size());

				if (ZSTD_isError(result))
				{
					return false;
				}

				return Qoi::Decode(scratch, dst);
			}

	};
#endif

/*
	class Qoi16;
	// class Qoi16nr;

	// Encode/decode the QOI image format. This implementation ignores alpha because ImageBase<> tends
	// to only be greyscale or RGB. It encodes into the supplied buffer and decodes into the supplied
	// image which promotes reuse of memory - useful when handling continuous streams of images.
	// In a change from the QOI standard, the last byte in the header is used to indicate the bit
	// depth of the image (typesize) instead of the colourspace.
	class Qoi8
	{
		public:


			template <typename C> static bool Encode(const ImageBase<byte> &src, C &dst)
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

				Qoi::Header header(width, height, depth, 1);

				dst.resize(width * height * (depth + 1) + sizeof(Qoi::Header) + sizeof(Qoi::PADDING));
				std::memcpy(dst.data(), &header, sizeof(Qoi::Header));

				std::array<Pixel, 64> index;
				Pixel previous, current;

				byte *pd	= dst.data() + sizeof(Qoi::Header);
				int run		= 0;

				for (auto *p : src.Pixels())
				{
					current.rgb.r = p[0];
					current.rgb.g = p[1];
					current.rgb.b = p[2];

					// We're not using alpha channels at all, so skip this
					// if (depth == 4)
					// {
					// 	current.rgb.a = p[3];
					// }

					if (current.v == previous.v)
					{
						if (++run == 62)
						{
							*pd++	= Qoi::OP_RUN | (run - 1);
							run		= 0;
						}
					}
					else
					{
						if (run)
						{
							*pd++	= Qoi::OP_RUN | (run - 1);
							run		= 0;
						}

						const int lookup = Hash(current);

						if (index[lookup].v == current.v)
						{
							*pd++ = Qoi::OP_INDEX | lookup;
						}
						else
						{
							index[lookup] = current;

							const signed char dr	= current.rgb.r - previous.rgb.r;
							const signed char dg	= current.rgb.g - previous.rgb.g;
							const signed char db	= current.rgb.b - previous.rgb.b;
							const signed char dgr	= dr - dg;
							const signed char dgb	= db - dg;

							if (dr > -3 && dr < 2 && dg > -3 && dg < 2 && db > -3 && db < 2)
							{
								*pd++ = Qoi::OP_DIFF | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2);
							}
							else if (dgr > -9 && dgr < 8 && dg > -33 && dg < 32 && dgb > -9 && dgb < 8)
							{
								*pd++ = Qoi::OP_LUMA | (dg + 32);
								*pd++ = (dgr + 8) << 4 | (dgb + 8);
							}
							else
							{
								*pd++ = Qoi::OP_RGB;
								*pd++ = current.rgb.r;
								*pd++ = current.rgb.g;
								*pd++ = current.rgb.b;
							}
						}
					}

					previous = current;
				}

				if (run)
				{
					*pd++ = Qoi::OP_RUN | (run - 1);
				}

				std::memcpy(pd, Qoi::PADDING.data(), sizeof(Qoi::PADDING));
				dst.resize(pd + sizeof(Qoi::PADDING) - dst.data());

				return true;
			}


			template <typename C> static bool Decode(const C &src, ImageBase<byte> &dst)
			{
				static_assert(is_contiguous<C>, "source must be a contiguous container type");
				static_assert(sizeof(typename C::value_type) == 1, "source must be a byte buffer");

				if (src.size() < sizeof(Qoi::Header) + sizeof(Qoi::PADDING))
				{
					return false;
				}

				Qoi::Header header;
				std::memcpy(&header, src.data(), sizeof(Qoi::Header));

				const size_t width	= be32toh(header.width);
				const size_t height	= be32toh(header.height);
				const byte depth	= header.depth;

				if (width == 0 || height == 0 || depth != 3 || header.typesize != 1 || header.magic != htobe32(Qoi::MAGIC))
				{
					return false;
				}

				if (width * height >= Qoi::MAX_PIXELS)
				{
					return false;
				}

				dst.Resize(width, height, depth);

				std::array<Pixel, 64> index;
				Pixel pixel;

				const byte *current	= src.data() + sizeof(Qoi::Header);
				const byte *end		= src.data() + src.size() - sizeof(Qoi::PADDING);
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

						if (op == Qoi::OP_RGB)
						{
							pixel.rgb.r = *current++;
							pixel.rgb.g = *current++;
							pixel.rgb.b = *current++;
						}
						else if ((op & Qoi::MASK) == Qoi::OP_INDEX)
						{
							pixel = index[op];
						}
						else if ((op & Qoi::MASK) == Qoi::OP_DIFF)
						{
							pixel.rgb.r += ((op >> 4) & 0x03) - 2;
							pixel.rgb.g += ((op >> 2) & 0x03) - 2;
							pixel.rgb.b += ( op       & 0x03) - 2;
						}
						else if ((op & Qoi::MASK) == Qoi::OP_LUMA)
						{
							const int o2 = *current++;
							const int dg = (op & 0x3f) - 32;

							pixel.rgb.r += dg - 8 + ((o2 >> 4) & 0x0f);
							pixel.rgb.g += dg;
							pixel.rgb.b += dg - 8 + ( o2       & 0x0f);
						}
						else if ((op & Qoi::MASK) == Qoi::OP_RUN)
						{
							run = op & 0x3f;
						}

						index[Hash(pixel)] = pixel;
					}

					p[0] = pixel.rgb.r;
					p[1] = pixel.rgb.g;
					p[2] = pixel.rgb.b;
					// ignoring alpha here
				}

				return true;
			}


		private:

			struct Header
			{
				uint32_t magic		= htobe32(MAGIC);
				uint32_t width		= 0;
				uint32_t height		= 0;
				byte depth			= 0;
				byte typesize		= 1;	// Making use of this field to store the image typesize


				Header() = default;
				Header(const uint32_t width, const uint32_t height, const byte depth, const byte typesize)
					: width(htobe32(width)), height(htobe32(height)), depth(depth), typesize(typesize) {}

			} __attribute__((packed));

			union Pixel
			{
				struct { byte r, g, b; } rgb;
				uint32_t v = 0xff000000;
			};

			static inline int Hash(const Pixel &p)
			{
				return (p.rgb.r * 3
					+ p.rgb.g * 5
					+ p.rgb.b * 7
					+ 255 * 11) % 64;
			}

			static constexpr byte OP_INDEX	= 0x00;
			static constexpr byte OP_DIFF	= 0x40;
			static constexpr byte OP_LUMA	= 0x80;
			static constexpr byte OP_RUN	= 0xc0;
			static constexpr byte OP_RGB	= 0xfe;
			// static constexpr byte OP_RGBA	= 0xff;
			static constexpr byte MASK		= 0xc0;

			static constexpr uint32_t MAGIC					= 'q' << 24 | 'o' << 16 | 'i' << 8 | 'f';
			static constexpr uint32_t MAX_PIXELS			= 400'000'000;
			static constexpr uint32_t HEADER_SIZE			= 14;
			static constexpr std::array<byte, 8> PADDING	= { 0, 0, 0, 0, 0, 0, 0, 1 };


			friend Qoi16;
			// friend Qoi16nr;
	};
*/


//======================================================================================================
//------------------------------------------------------------------------------------------------------
//======================================================================================================


/*
	// Qoi with residuals. Use the standard Qoi method for the upper byte of each channel and then
	// encode the lower bytes (residuals) directly. This poses an issue with the run length encoding
	// because decoding will require the opcode first - so cache the residuals and then push them
	// sequentially into the destination after the run op.
	// This algorithm is based on the assumption that HDR images contain more noise in the lower
	// bits which makes this kind of encoding more difficult, so instead concentrate on shrinking
	// the most significant bits instead.
	class Qoi16
	{
		public:

			static constexpr bool DEBUG = false;

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

				Qoi::Header header(width, height, depth, 2);

				dst.resize(width * height * (depth + 1) * 2 + sizeof(Qoi::Header) + sizeof(Qoi::PADDING));
				std::memcpy(dst.data(), &header, sizeof(Qoi::Header));

				std::array<Pixel, LOOKUP_SIZE> index = {{}};
				std::array<Pixel, RUN_SIZE+1> residuals;
				Pixel previous, current;


				byte *pd	= dst.data() + sizeof(Qoi::Header);
				int run		= 0;

				std::array<int, 5> opCount = { 0 };

				for (auto *p : src.Pixels())
				{
					current.rgb.r			= p[0] >> 8;
					current.rgb.g			= p[1] >> 8;
					current.rgb.b			= p[2] >> 8;
					residuals[run].rgb.r	= p[0] & 0xff;
					residuals[run].rgb.g	= p[1] & 0xff;
					residuals[run].rgb.b	= p[2] & 0xff;

				 	if (current.v == previous.v)
				 	{
				 		if (++run == RUN_SIZE)
				 		{
				 			if constexpr (DEBUG) opCount[0]++;
				 			pd	= Run(pd, run, residuals);
				 			run	= 0;
				 		}
				 	}
				 	else
				 	{
				 		const auto &res	= residuals[run];

				 		if (run)
				 		{
				 			if constexpr (DEBUG) opCount[0]++;
				 			pd	= Run(pd, run, residuals);
					 		run	= 0;
				 		}

						const int lookup = Hash(current);

						if (index[lookup].v == current.v)
						{
							if constexpr (DEBUG) opCount[1]++;
							*pd++ = OP_INDEX | lookup;
						}
						else
						{
							index[lookup] = current;

							const signed char dr	= current.rgb.r - previous.rgb.r;
							const signed char dg	= current.rgb.g - previous.rgb.g;
							const signed char db	= current.rgb.b - previous.rgb.b;
							const signed char dgr	= dr - dg;
							const signed char dgb	= db - dg;

							if (dr > -3 && dr < 2 && dg > -3 && dg < 2 && db > -3 && db < 2)
							{
								if constexpr (DEBUG) opCount[2]++;
								*pd++ = OP_DIFF | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2);
							}
							else if (dgr > -9 && dgr < 8 && dg > -33 && dg < 32 && dgb > -9 && dgb < 8)
							{
								if constexpr (DEBUG) opCount[3]++;
								*pd++ = OP_LUMA | (dg + 32);
								*pd++ = (dgr + 8) << 4 | (dgb + 8);
							}
							else
							{
								if constexpr (DEBUG) opCount[4]++;
								*pd++ = OP_RGB;
								*pd++ = current.rgb.r;
								*pd++ = current.rgb.g;
								*pd++ = current.rgb.b;
							}


						}

						*pd++ = res.rgb.r;
						*pd++ = res.rgb.g;
						*pd++ = res.rgb.b;
					}

					previous = current;
				}

				if (run)
		 		{
		 			if constexpr (DEBUG) opCount[0]++;
		 			pd = Run(pd, run, residuals);
		 		}

				std::memcpy(pd, Qoi::PADDING.data(), sizeof(Qoi::PADDING));
				dst.resize(pd + sizeof(Qoi::PADDING) - dst.data());


				if constexpr (DEBUG)
				{
					const int total = std::accumulate(opCount.begin(), opCount.end(), 0);

					std::cout
						<< "  pixels=" << width * height * depth
						<< "  ops=" << total
						<< "  run=" << Print(opCount[0], total)
						<< "  index=" << Print(opCount[1], total)
						<< "  diff=" << Print(opCount[2], total)
						<< "  luma=" << Print(opCount[3], total)
						<< "  rgb=" << Print(opCount[4], total)
						<< "  ratio=" << ((double)dst.size() / (double)(width * height * depth * 2))
						<< '\n';

					// output for spreadsheet
					std::cout << width * height * depth << ' ' << total
						<< PrintRaw(opCount[0], total)
						<< PrintRaw(opCount[1], total)
						<< PrintRaw(opCount[2], total)
						<< PrintRaw(opCount[3], total)
						<< PrintRaw(opCount[4], total)
						<< ' ' << ((double)dst.size() / (double)(width * height * depth * 2))
						<< '\n';
				}

				return true;
			}

			static string Print(const int value, const int total)
			{
				return emg::String::format("%d (%.3f)", value, (double)value / (double)total);
			}

			static string PrintRaw(const int value, const int total)
			{
				return emg::String::format(" %d %.3f", value, (double)value / (double)total);
			}


			template <typename C> static bool Decode(const C &src, ImageBase<uint16_t> &dst)
			{
				static_assert(is_contiguous<C>, "source must be a contiguous container type");
				static_assert(sizeof(typename C::value_type) == 1, "source must be a byte buffer");

				if (src.size() < sizeof(Qoi::Header) + sizeof(Qoi::PADDING))
				{
					return false;
				}

				Qoi::Header header;
				std::memcpy(&header, src.data(), sizeof(Qoi::Header));

				const size_t width	= be32toh(header.width);
				const size_t height	= be32toh(header.height);
				const byte depth	= header.depth;

				if (width == 0 || height == 0 || depth != 3 || header.typesize != 2 || header.magic != htobe32(Qoi::MAGIC))
				{
					return false;
				}

				if (width * height >= Qoi::MAX_PIXELS)
				{
					return false;
				}

				dst.Resize(width, height, depth);

				// std::array<Residual, LOOKUP_SIZE> index = {{}};
				// Residual pixel;
				std::array<Pixel, LOOKUP_SIZE> index = {{}};
				Pixel pixel;

				const byte *current	= src.data() + sizeof(Qoi::Header);
				const byte *end		= src.data() + src.size() - sizeof(Qoi::PADDING);
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
							pixel.rgb.r = *current++;
							pixel.rgb.g = *current++;
							pixel.rgb.b = *current++;
						}
						else if ((op & MASK) == OP_INDEX)
						{
							pixel = index[op];
						}
						else if ((op & MASK) == OP_DIFF)
						{
							pixel.rgb.r += ((op >> 4) & 0x03) - 2;
							pixel.rgb.g += ((op >> 2) & 0x03) - 2;
							pixel.rgb.b += ( op       & 0x03) - 2;
						}
						else if ((op & MASK) == OP_LUMA)
						{
							const int o2 = *current++;
							const int dg = (op & 0x3f) - 32;

							pixel.rgb.r += dg - 8 + ((o2 >> 4) & 0x0f);
							pixel.rgb.g += dg;
							pixel.rgb.b += dg - 8 + ( o2       & 0x0f);
						}
						else if ((op & MASK) == OP_RUN)
						{
							run = op & 0x3f;
						}

						index[Hash(pixel)] = pixel;
					}

					p[0] = (pixel.rgb.r << 8) | *current++;
					p[1] = (pixel.rgb.g << 8) | *current++;
					p[2] = (pixel.rgb.b << 8) | *current++;
					// ignoring alpha here
				}

				return true;
			}


		private:

			static constexpr byte OP_INDEX	= 0x00;	// 0b0000'0000;
			static constexpr byte OP_DIFF	= 0x40; // 0b0100'0000;
			static constexpr byte OP_LUMA	= 0x80; // 0b1000'0000;
			static constexpr byte OP_RUN	= 0xc0; // 0b1100'0000;
			static constexpr byte OP_RGB	= 0xfe;	// 0b1111'1110;
			static constexpr byte OP_RGBA	= 0xff;	// 0b1111'1111;
			static constexpr byte MASK		= 0xc0;	// 0b1100'0000;

			static constexpr size_t LOOKUP_SIZE = 64;	// Size of the pixel lookup
			static constexpr size_t RUN_SIZE	= 62;	// Size of the run length

			union Pixel
			{
				struct { byte r, g, b; } rgb;
				uint32_t v = 0;
			};

			static inline int Hash(const Pixel &p)
			{
				return (p.rgb.r * 3
					+ p.rgb.g * 5
					+ p.rgb.b * 7
					+ 255 * 11) % 64;
			}

			// Write a run length + residuals block to the buffer
			static byte *Run(byte *dst, const int run, const std::array<Pixel, RUN_SIZE+1> &residuals)
			{
				*dst++	= OP_RUN | (run - 1);

				for (int i=0; i<run; i++)
				{
					*dst++ = residuals[i].rgb.r;
					*dst++ = residuals[i].rgb.g;
					*dst++ = residuals[i].rgb.b;
				}

				return dst;
			}
	};
*/

//======================================================================================================
//------------------------------------------------------------------------------------------------------
//======================================================================================================

	// Non-residual version of the 16-bit Qoi. This attempts to encode the entire pixel
	// value instead of just the upper byte, but due to the tendency of 16-bit data to
	// contain more noise this often results in worse compression than the standard residual
	// version (although not always). No decoder implementation yet
/*	class Qoi16nr
	{
		public:

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

						if (index[lookup].v == current.v)
						{
							opCount[2]++;
							*pd++ = OP_INDEX | (lookup) >> 8;
							*pd++ = lookup & 0xff;
						}
						else
						{
							index[lookup]	= current;
							const int dr	= current.rgba.r - previous.rgba.r;
							const int dg	= current.rgba.g - previous.rgba.g;
							const int db	= current.rgba.b - previous.rgba.b;
							const int dgr	= dr - dg;
							const int dgb	= db - dg;

							if (dg > -65 && dg < 64 && dr > -65 && dr < 64 && db > -65 && db < 64)
							{
								opCount[3]++;
								*pd++ = OP_DIFF | ((dr + 64) >> 5);
								*pd++ = ((dr + 64) & 0x03) << 6 | (((dg + 64) & 0x3f) >> 1);
								*pd++ = (((dg + 64) & 0x40) << 1) | (db + 64);
							}
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

				// output for spreadsheet
				std::cout << width * height * depth << ' ' << total
					<< PrintRaw(opCount[0] + opCount[1], total)
					<< PrintRaw(opCount[2], total)
					<< PrintRaw(opCount[3], total)
					<< PrintRaw(opCount[4], total)
					<< PrintRaw(opCount[5], total)
					<< ' ' << ((double)dst.size() / (double)(width * height * depth * 2))
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

			static string PrintRaw(const int value, const int total)
			{
				return emg::String::format(" %d %.3f", value, (double)value / (double)total);
			}


			template <typename C> static bool Decode(const C &src, ImageBase<uint16_t> &dst)
			{
				return false;
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

			static constexpr size_t LOOKUP_SIZE = 4095;	// Size of the pixel lookup
			static constexpr size_t RUN_SIZE	= 4096;	// Size of the run length

			union Pixel
			{
				struct { uint16_t r, g, b; } rgba;
				uint64_t v = 0;
			};


			static inline int Hash(const Pixel &p)
			{
				return (p.rgba.r * 13
					+ p.rgba.g * 37
					+ p.rgba.b * 67
				) % LOOKUP_SIZE;
			}
	};
*/

}
