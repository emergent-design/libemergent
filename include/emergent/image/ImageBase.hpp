#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/image/Colours.hpp>
#include <emergent/struct/Buffer.hpp>
#include <emergent/struct/Distribution.hpp>
#include <FreeImage.h>


namespace emergent
{
	// A simple header used when storing a raw Image<> as a binary
	// blob (for example, to file or Redis).
	struct ImageHeader
	{
		byte depth;
		byte typesize;
		uint16_t width;
		uint16_t height;
	};


	/// Base class for images of a given arithmetic type.
	template <class T = byte> class ImageBase
	{
		static_assert(std::is_arithmetic<T>::value, "Image type must be numeric or boolean");

		public:
			/// Destructor
			virtual ~ImageBase() { }

			/// Operator override to support implicit and explicit typecasting
			operator T*() { return this->buffer; }

			/// Return the buffer data
			T *Data() { return this->buffer; }

			/// Return the actual buffer.
			/// WARNING: If you modify the size of this buffer you will corrupt the owner image
			Buffer<T> *Internal() { return &this->buffer; }

			/// Return the image type ID
			int Type() { return this->type; }

			/// Return the image depth (in bytes)
			int Depth() { return this->depth; }

			/// Return the image width
			int Width() { return this->width; }

			/// Return the image height
			int Height() { return this->height; }

			/// Return the image size
			int Size() { return this->width * this->height; }

			/// Clear the image - sets all pixel values to 0 regardless of type
			void Clear() { this->buffer.Clear(); }


			/// Resizes the image buffers but destroys any existing image data.
			void Resize(int width, int height)
			{
				if (width > 0 && height > 0)
				{
					this->width		= width;
					this->height	= height;
					this->buffer.Resize(width * height * this->depth);
				}
				this->lookup.Resize(0);
			}


			/// Returns the maximum value in the current image data (regardless of image depth).
			T Max()
			{
				T result 	= std::numeric_limits<T>::min();
				T *src		= this->buffer;
				int size	= this->buffer.Size();

				for (int i=0; i<size; i++, src++) if (*src > result) result = *src;

				return result;
			}


			/// Returns the minimum value in the current image data (regardless of image depth).
			T Min()
			{
				T result 	= std::numeric_limits<T>::max();
				T *src		= this->buffer;
				int size	= this->buffer.Size();

				for (int i=0; i<size; i++, src++) if (*src < result) result = *src;

				return result;
			}


			/// Truncate the height of the image down to the given height. Since this just requires
			/// a change of the height value and a cut of the buffer it allows you to chop the
			/// bottom off of an image without needing to use an additional image buffer.
			bool Truncate(int height)
			{
				if (height < this->height && height > 0)
				{
					if (this->buffer.Truncate(height * this->width * this->depth))
					{
						this->height = height;
						return true;
					}
				}

				return false;
			}


			/// Count the number of values in the current image data (regardless of image depth)
			/// that match the supplied predicate.
			int Count(std::function<bool(T value)> predicate)
			{
				int result	= 0;
				int size	= this->buffer.Size();
				T *src		= this->buffer;

				for (int i=0; i<size; i++) if (predicate(*src++)) result++;

				return result;
			}


			/// Count the number of zero values in the current image data (regardless of image depth)
			int ZeroCount()
			{
				int result	= 0;
				int size	= this->buffer.Size();
				T *src		= this->buffer;

				for (int i=0; i<size; i++) if (!*src++) result++;

				return result;
			}

			///Check if all the pixels in the image are set to the same value (regardless of image depth)
			bool IsBlank(T reference = 0)
			{
				T* src 		= this->buffer;
				int size 	= this->buffer.Size();
				for (int i = 0; i < size; i++)
				{
					if(*src++ != reference) return false;
				}
				return true;
			}


			/// Clamp the current image data (regardless of image depth) to the supplied
			/// lower (black) and upper (white) limits.
			void Clamp(T black, T white)
			{
				T *src 		= this->buffer;
				int size	= this->width * this->height * this->depth;

				for(int i = 0; i< size; i++, src++)
				{
					*src = *src < black ? black : (*src > white ? white : *src);
				}
			}


			/// Load an image from file. Uses the freeimage library to attempt loading
			/// and conversion of the image. It should cope with any standard image
			/// formats.
			bool Load(std::string path)
			{
				bool result = false;
				auto fif	= FreeImage_GetFileType(path.c_str(), 0);

				if (fif == FIF_UNKNOWN) fif = FreeImage_GetFIFFromFilename(path.c_str());

				if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsReading(fif))
				{
					auto *image = FreeImage_Load(fif, path.c_str());

					if (image)
					{
						result = this->FromFib(image);
						FreeImage_Unload(image);
					}
				}

				return result;
			}


			/// Load an image from memory buffer
			bool Load(Buffer<byte> &buffer)
			{
				bool result	= false;
				auto *mem 	= FreeImage_OpenMemory(buffer.Data(), buffer.Size());

				if (mem)
				{
					auto fif = FreeImage_GetFileTypeFromMemory(mem, 0);

					if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsReading(fif))
					{
						auto *image = FreeImage_LoadFromMemory(fif, mem);

						if (image)
						{
							result = this->FromFib(image);
							FreeImage_Unload(image);
						}
					}

					FreeImage_CloseMemory(mem);
				}

				return result;
			}


			/// Save an image to file. Uses the freeimage library to attempt saving
			/// out the image (converts it to byte first but does not scale the values,
			/// so be warned). Image format is automatically determined by file extension.
			bool Save(std::string path, int compression = 0)
			{
				bool result = false;

				if (this->Size())
				{
					auto *image = this->ToFib();

					if (image)
					{
						auto fif = FreeImage_GetFIFFromFilename(path.c_str());

						if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsWriting(fif))
						{
							switch (compression)
							{
								case 0:		result = FreeImage_Save(fif, image, path.c_str(), PNG_DEFAULT);			break;
								case 1:		result = FreeImage_Save(fif, image, path.c_str(), JPEG_QUALITYGOOD);	break;
								default:	result = FreeImage_Save(fif, image, path.c_str(), JPEG_QUALITYNORMAL);	break;
							}
						}

						FreeImage_Unload(image);
					}
				}

				return result;
			}



			/// Save image to a memory buffer.
			bool Save(Buffer<byte> &buffer, int compression)
			{
				bool result = false;

				if (this->Size())
				{
					DWORD size;
					byte *data;
					auto *mem 	= FreeImage_OpenMemory();
					auto *image = this->ToFib();

					if (image)
					{
						switch (compression)
						{
							case 0:		result = FreeImage_SaveToMemory(FIF_PNG, image, mem, PNG_DEFAULT);			break;
							case 1:		result = FreeImage_SaveToMemory(FIF_JPEG, image, mem, JPEG_QUALITYGOOD);	break;
							default:	result = FreeImage_SaveToMemory(FIF_JPEG, image, mem, JPEG_QUALITYNORMAL);	break;
						}

						FreeImage_Unload(image);
					}

					if (result && FreeImage_AcquireMemory(mem, &data, &size))
					{
						buffer.Set(data, size);
					}
					else result = false;

					FreeImage_CloseMemory(mem);
				}

				return result;
			}


			/// Load a raw image file, expects ImageHeader to be at the beginning
			bool LoadRaw(std::string path)
			{
				ImageHeader h;
				std::ifstream ifs(path, std::ios::in | std::ios::binary);

				if (ifs.good())
				{
					int size = (int)ifs.seekg(0, std::ios::end).tellg();
					ifs.seekg(0);

					if (size > sizeof(ImageHeader))
					{
						ifs.read((char *)&h, sizeof(ImageHeader));

						int length = h.width * h.height * h.depth * sizeof(T);

						if (h.typesize == sizeof(T) && h.depth == this->depth && size == sizeof(ImageHeader) + length)
						{
							this->Resize(h.width, h.height);
							ifs.read((char *)this->Data(), size);

							return true;
						}
					}
				}

				return false;
			}


			/// Save a raw image file starting with ImageHeader
			bool SaveRaw(std::string path)
			{
				if (this->Size())
				{
					std::ofstream ofs(path, std::ios::out | std::ios::binary);

					if (ofs.good())
					{
						ImageHeader header = { this->Depth(), sizeof(T), (uint16_t)this->width, (uint16_t)this->height };

						ofs.write((char *)&header, sizeof(ImageHeader));
						ofs.write((char *)this->Data(), this->width * this->height * this->depth * sizeof(T));
						ofs.flush();

						return true;
					}
				}

				return false;
			}


			/// Calculate the distribution statistics of the image
			/// mask can be optionally passed in; zero values in the mask tell distribution to
			/// ignore the corresponding pixels in the image.
			distribution Stats(Buffer<byte> *mask = nullptr)
			{
				return distribution(this->buffer, mask);
			}


			/// Threshold this image at the given value (each channel is thresholded independently)
			void Threshold(T value, bool above = true)
			{
				int size	= this->width * this->height * this->depth;
				T *src		= this->buffer;

				if (above)	for (int i=0; i<size; i++, src++) 	*src = (*src < value) ? 0 : 255;
				else		for (int i=0; i<size; i++, src++)	*src = (*src > value) ? 0 : 255;
			}


			/// Shift all of the values in the image data (regardless of image depth) by the
			/// specified amount.
			void Shift(int value)
			{
				int size	= this->width * this->height * this->depth;
				T *src		= this->buffer;

				for (int i=0; i<size; i++, src++) *src = Maths::clamp<T>(*src + value);
			}


			/// Gets the value of a pixel for a specific channel but supports values outside the image dimensions,
			/// if mirror is true the pixel values are mirrored, otherwise they are smeared. Weird things will happen
			/// if a value that is twice the width/height outside the image is requested.
			T Value(int x, int y, byte channel = 0, bool mirror = true)
			{
				if (channel < this->depth)
				{
					int ax 		= x < 0 ? (mirror ? -x : 0) : x < this->width  ? x : (mirror ? this->width + this->width - x - 2 : this->width - 1);
					int ay 		= y < 0 ? (mirror ? -y : 0) : y < this->height ? y : (mirror ? this->height + this->height - y - 2 : this->height - 1);
					int line	= this->width * this->depth;

					return *(this->buffer + ay * line + ax * this->depth + channel);
				}

				return 0;
			}


			/// Interpolate the pixel value of a specific channel at the given coordinates.
			T Interpolate(double x, double y, byte channel = 0)
			{
				if (x >= 0 && x < this->width - 1 && y >= 0 && y < this->height - 1 && channel < this->depth)
				{
					int line	= this->width * this->depth;
					double sx	= x - (int)x;
					double sy	= y - (int)y;
					T *pb		= this->buffer + (int)y * line + (int)x * this->depth + channel;

					return lrint(
						(double)*pb * (1 - sx) * (1 - sy) +
						(double)*(pb + this->depth) * sx * (1 - sy) +
						(double)*(pb + line) * (1 - sx) * sy +
						(double)*(pb + line + this->depth) * sx * sy
					);
				}

				return 0;
			}


			/// Variance normalise the image to a target variance.
			/// Each channel is normalised independently.
			bool VarianceNormalise(double targetVariance)
			{
				int size = this->Size();

				if (size)
				{
					T *b;
					int i, j;
					double mean, variance;
					double sum[Depth::max] = {}, squared[Depth::max] = {}, scale[Depth::max], shift[Depth::max];

					for (i=0, b=buffer; i<size; i++)
					{
						for (j=0; j<this->depth; j++)
						{
							squared[j]	+= *b * *b;
							sum[j]		+= *b++;
						}
					}

					for (i=0; i<this->depth; i++)
					{
						mean		= sum[i] / (double)size;
						variance	= (squared[i] / (double)size) - mean * mean;
						scale[i]	= sqrt(targetVariance / variance);
						shift[i]	= 128 - (mean * scale[i]);
					}

					for (i=0, b=buffer; i<size; i++)
					{
						for (j=0; j<this->depth; j++, b++)
						{
							*b = Maths::clamp<T>(lrint((double)*b * scale[j] + shift[j]));
						}
					}

					return true;
				}

				return false;
			}


			/// Normalise the image data. Each channel is normalised
			/// independently.
			bool Normalise()
			{
				int size = this->Size();

				if (size)
				{
					T *b;
					int i, j;
					double scale[Depth::max];
					T low[Depth::max], high[Depth::max];

					std::fill_n(low, Depth::max, std::numeric_limits<T>::max());
					std::fill_n(high, Depth::max, std::numeric_limits<T>::min());

					for (i=0, b=buffer; i<size; i++)
					{
						for (j=0; j<this->depth; j++)
						{
							low[j]	= std::min(low[j], *b);
							high[j]	= std::max(high[j], *b++);
						}
					}

					for (i=0; i<this->depth; i++)
					{
						scale[i] = 255.0 / std::max(1.0, (double)(high[i] - low[i]));
					}

					for (i=0, b=buffer; i<size; i++)
					{
						for (j=0; j<this->depth; j++, b++)
						{
							*b = lrint(scale[j] * (*b - low[j]));
						}
					}

					return true;
				}

				return false;
			}


			/// Generate a row lookup table
			T **Lookup()
			{
				if (this->lookup.Size() != this->height)
				{
					this->lookup.Resize(this->height);
					int line	= this->width * this->depth;
					T *pb		= this->buffer;
					T **pl		= this->lookup;

					for (int y=0; y<this->height; y++, pb+=line, pl++) *pl = pb;
				}

				return this->lookup;
			}


		protected:
			ImageBase(int type, int depth, int width = 0, int height = 0)
				: type(type), depth(depth), width(width), height(height), buffer(width * height * depth), lookup() {}

			/// Image type
			int type;

			/// Image depth
			int depth;

			/// Image width
			int width;

			/// Image height
			int height;

			/// Buffer for the actual image data
			Buffer<T> buffer;

			/// Lookup table for rows
			Buffer<T*> lookup;

		private:

			inline void SWAPCHANNELS(T& a, T& b) { a ^= b; b ^= a; a ^= b; }


			template <typename U = T> typename std::enable_if<std::is_same<byte, U>::value, FIBITMAP *>::type ToFib()
			{
				auto *result = FreeImage_ConvertFromRawBits(this->buffer, this->width, this->height, this->width * this->depth, this->depth * 8, 0, 0, 0, true);

				#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
					if (result && this->depth > 1)
					{
						int x, y;
						byte *data	= FreeImage_GetBits(result);
						int jump	= FreeImage_GetPitch(result) - this->width * this->depth;

						for (y=0; y<this->height; y++, data += jump)
						{
							for (x=0; x<this->width; x++, data += this->depth)
							{
								SWAPCHANNELS(data[0], data[2]);
							}
						}
					}
				#endif

				return result;
			}


			template <typename U = T> typename std::enable_if<!std::is_same<byte, U>::value, FIBITMAP *>::type ToFib()
			{
				FIBITMAP *result							= nullptr;
				std::function<int(T *src, byte *dst)> apply	= nullptr;

				if (std::is_integral<T>::value)
				{
					if (this->depth == 3)
					{
						result	= FreeImage_AllocateT(FIT_RGB16, this->width, this->height);
						apply	= [](T *src, byte *dst) {
							reinterpret_cast<FIRGB16 *>(dst)->red	= src[0];
							reinterpret_cast<FIRGB16 *>(dst)->green	= src[1];
							reinterpret_cast<FIRGB16 *>(dst)->blue	= src[2];
							return sizeof(FIRGB16);
						};
					}
					else if (this->depth == 1)
					{
						result	= FreeImage_AllocateT(FIT_UINT16, this->width, this->height);
						apply	= [](T *src, byte *dst) {
							*reinterpret_cast<ushort *>(dst) = *src;
							return sizeof(ushort);
						};
					}
				}
				else if (std::is_floating_point<T>::value)
				{
					if (this->depth == 3)
					{
						result	= FreeImage_AllocateT(FIT_RGBF, this->width, this->height);
						apply	= [](T *src, byte *dst) {
							reinterpret_cast<FIRGBF *>(dst)->red	= src[0];
							reinterpret_cast<FIRGBF *>(dst)->green	= src[1];
							reinterpret_cast<FIRGBF *>(dst)->blue	= src[2];
							return sizeof(FIRGBF);
						};
					}
					else if (this->depth == 1)
					{
						result	= FreeImage_AllocateT(FIT_FLOAT, this->width, this->height);
						apply	= [](T *src, byte *dst) {
							*reinterpret_cast<float *>(dst) = *src;
							return sizeof(float);
						};
					}
				}

				if (result)
				{
					int x, y;
					T *b = this->buffer;

					for (y=0; y<height; y++)
					{
						auto bits = FreeImage_GetScanLine(result, height - y - 1);

						for (x=0; x<width; x++)
						{
							bits	+= apply(b, bits);
							b		+= this->depth;
						}
					}
				}

				return result;
			}



			template <typename U = T> typename std::enable_if<std::is_same<byte, U>::value, bool>::type FromFib(FIBITMAP *image)
			{
				this->width 	= FreeImage_GetWidth(image);
				this->height	= FreeImage_GetHeight(image);
				this->buffer.Resize(this->width * this->height * this->depth);

				FreeImage_ConvertToRawBits(this->buffer, image, this->width * this->depth, this->depth * 8, 0, 0, 0, true);

				#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
					if (this->depth > 1)
					{
						byte *end = this->buffer + this->width * this->height * this->depth;

						for (byte *data = this->buffer; data < end; data += this->depth) SWAPCHANNELS(data[0], data[2]);
					}
				#endif

				return true;
			}


			template <typename U = T> typename std::enable_if<!std::is_same<byte, U>::value, bool>::type FromFib(FIBITMAP *image)
			{
				FIBITMAP *converted							= nullptr;
				std::function<int(byte *src, T *dst)> apply = nullptr;

				this->width 	= FreeImage_GetWidth(image);
				this->height	= FreeImage_GetHeight(image);
				this->buffer.Resize(this->width * this->height * this->depth);

				if (std::is_integral<T>::value)
				{
					if (this->depth == 3)
					{
						converted 	= FreeImage_ConvertToRGB16(image);
						apply		= [](byte *src, T *dst) {
							dst[0] = reinterpret_cast<FIRGB16 *>(src)->red;
							dst[1] = reinterpret_cast<FIRGB16 *>(src)->green;
							dst[2] = reinterpret_cast<FIRGB16 *>(src)->blue;
							return sizeof(FIRGB16);
						};
					}
					else if (this->depth == 1)
					{
						converted	= FreeImage_ConvertToUINT16(image);
						apply		= [](byte *src, T *dst) {
							*dst = *reinterpret_cast<ushort *>(src);
							return sizeof(ushort);
						};
					}
				}
				else if (std::is_floating_point<T>::value)
				{
					if (this->depth == 3)
					{
						converted	= FreeImage_ConvertToRGBF(image);
						apply		= [](byte *src, T *dst) {
							dst[0] = reinterpret_cast<FIRGBF *>(src)->red;
							dst[1] = reinterpret_cast<FIRGBF *>(src)->green;
							dst[2] = reinterpret_cast<FIRGBF *>(src)->blue;
							return sizeof(FIRGBF);
						};
					}
					else if (this->depth == 1)
					{
						converted	= FreeImage_ConvertToFloat(image);
						apply		= [](byte *src, T *dst) {
							*dst = *reinterpret_cast<float *>(src);
							return sizeof(float);
						};
					}
				}

				if (converted)
				{
					int x, y;
					T *b = this->buffer;

					for (y=0; y<height; y++)
					{
						auto bits = FreeImage_GetScanLine(converted, height - y - 1);

						for (x=0; x<width; x++)
						{
							bits	+= apply(bits, b);
							b		+= this->depth;
						}
					}

					FreeImage_Unload(converted);

					return true;
				}

				return false;
			}
	};
}
