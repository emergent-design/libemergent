#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/image/Colours.hpp>
#include <emergent/struct/Buffer.hpp>
#include <emergent/struct/Distribution.hpp>
#include <emergent/image/Operations.hpp>
#include <FreeImage.h>
#include <cstring>

#include <iostream>

namespace emergent
{
	// A simple header used when storing a raw Image<> as a binary blob (for example, to file or Redis).
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


			ImageBase(byte depth = 1, int width = 0, int height = 0) : depth(depth), width(width), height(height)
			{
				if (!depth) throw std::runtime_error("Image depth must be greater than zero");

				if (width && height) this->buffer.Resize(width * height * depth);
			}


			ImageBase(std::string path, byte depth = 1)
			{
				if (!depth) throw std::runtime_error("Image depth must be greater than zero");

				this->Load(path, depth);
			}


			//The depth of the source image is copied.
			ImageBase(const ImageBase<T> &image)
			{
				this->depth = image.depth;
				this->Copy(image);
			}


			/// Copy constructor with automatic type conversion. The depth
			/// of the source image is copied.
			template <class U> ImageBase(const ImageBase<U> &image)
			{
				this->depth = image.depth;
				this->Copy(image);
			}


			virtual ~ImageBase() {}


			/// Assignment override
			ImageBase<T> &operator=(const ImageBase<T> &image)
			{
				this->depth = image.depth;
				this->Copy(image);
				return *this;
			}


			/// Assignment override with type conversion.
			template <class U> ImageBase<T> &operator=(const ImageBase<U> &image)
			{
				this->depth = image.depth;
				this->Copy(image);
				return *this;
			}


			/// Assignment override which will set all pixels in the image to the given value
			/// Can be used to clear all the pixels of an RGB image to 0,0,0 for example.
			ImageBase<T> &operator=(const T value)
			{
				this->buffer = value;
				return *this;
			}


			/// Operator override to support implicit and explicit typecasting
			operator T*() const { return this->buffer; }

			/// Return the buffer data
			T *Data() const { return this->buffer; }

			/// Return the actual internal buffer.
			/// WARNING: If you modify the size of this buffer you will corrupt the owner image
			Buffer<T> &Internal() { return this->buffer; }

			/// Return the image type ID
			//int Type() { return this->type; }

			/// Return the image depth (in bytes)
			byte Depth() const { return this->depth; }

			/// Return the image width
			int Width() const { return this->width; }

			/// Return the image height
			int Height() const { return this->height; }

			/// Return the image size
			int Size() const { return this->width * this->height; }

			/// Clear the image - sets all pixel values to 0 regardless of type
			void Clear() { this->buffer.Clear(); }


			/// Resizes the image buffers but destroys any existing image data.
			/// A depth of 0 will keep the existing depth.
			virtual void Resize(int width, int height, byte depth = 0)
			{
				if (width > 0 && height > 0)
				{
					this->width		= width;
					this->height	= height;
					this->depth		= depth ? depth : this->depth;
					this->buffer.Resize(width * height * this->depth);
				}
			}


			/// Returns the maximum value in the current image data (regardless of image depth).
			T Max() const { return Operations::Max(this->buffer); }

			/// Returns the minimum value in the current image data (regardless of image depth).
			T Min() const { return Operations::Min(this->buffer); }

			/// Count the number of values in the current image data (regardless of image depth)
			/// that match the supplied predicate.
			int Count(std::function<bool(T value)> predicate) const { return Operations::Count(this->buffer, predicate); }

			/// Count the number of zero values in the current image data (regardless of image depth)
			int ZeroCount() const { return Operations::ZeroCount(this->buffer); }

			///Check if all the pixels in the image are set to the same value (regardless of image depth)
			bool IsBlank(T reference = 0) const { return Operations::IsBlank(this->buffer, reference); }

			/// Clamp the current image data (regardless of image depth) to the supplied
			/// lower and upper limits.
			void Clamp(T lower, T upper) { Operations::Clamp(this->buffer, lower, upper); }

			/// Shift all of the values in the image data (regardless of image depth) by the
			/// specified amount.
			void Shift(int value) { Operations::Shift(this->buffer, value); }

			/// Threshold this image at the given value
			void Threshold(T threshold, T high = 255, T low = 0) { Operations::Threshold(this->buffer, threshold, high, low); }

			/// Inverts this image
			void Invert() { Operations::Invert(this->buffer); }


			/// Arimetic OR of this image with a modifier image. The images should be of the
			/// same depth, but do not have to be as long as the underlying buffer sizes are
			/// the same. If they are not then an exception will be thrown.
			void OR(ImageBase<T> &modifier) { Operations::OR(this->buffer, modifier.buffer); }


			/// Arimetic AND of this image with a modifier image. The images should be of the
			/// same depth, but do not have to be as long as the underlying buffer sizes are
			/// the same. If they are not then an exception will be thrown.
			void AND(ImageBase<T> &modifier) { Operations::AND(this->buffer, modifier.buffer); }


			/// Variance normalise the image to a target variance. Each channel is normalised independently
			/// (although it assumes only greyscale and RGB).
			bool VarianceNormalise(double targetVariance)
			{
				return this->depth == 3
					? Operations::VarianceNormalise<3>(this->buffer, targetVariance)
					: Operations::VarianceNormalise<1>(this->buffer, targetVariance);
			}

			/// Normalise the image data. Each channel is normalised independently (although it assumes only
			/// greyscale and RGB).
			bool Normalise()
			{
				return this->depth == 3
					? Operations::Normalise<3>(this->buffer)
					: Operations::Normalise<1>(this->buffer);
			}


			/// Calculate the distribution statistics of the image mask can be optionally passed in;
			/// zero values in the mask tell distribution to ignore the corresponding pixels in the image.
			distribution Stats(Buffer<byte> *mask = nullptr) const
			{
				return distribution(this->buffer, mask);
			}


			// Apply an operation to inspect a given region of the image. The region must be fully
			// contained within the image and if it is invalid then `operation` will not be invoked.
			void Inspect(int rx, int ry, int rw, int rh, std::function<void(const T*)> operation) const
			{
				if (rx < 0 || rx+rw > this->width || ry < 0 || ry+rh > this->height)
				{
					return;
				}

				int x, y;
				const int depth = this->depth;
				const int jump	= (this->width - rw) * depth;
				T *pb			= this->buffer + (this->width * ry + rx) * depth;

				for (y=0; y<rh; y++, pb+=jump)
				{
					for (x=0; x<rw; x++, pb+=depth)
					{
						operation(pb);
					}
				}
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


			/// Add the values from another image to this one at the given offset
			/// Any values that drop off the edges are ignored. If sum is true then
			/// values are summed into the destination. Only image depths of 3 and 1 are
			/// supported unless sum is false and the depths are the same.
			ImageBase<T> &Insert(const ImageBase<T> &image, int x, int y, bool sum = false)
			{
				if (x >= 0 && x < this->width && y >= 0 && y < this->height)
				{
					int i, j;
					int ds	= this->depth;
					int di	= image.depth;
					int ls	= this->width * ds;
					int li	= image.width * di;
					int w	= std::min(image.width, this->width - x);
					int h	= std::min(image.height, this->height - y);
					T *ps 	= this->buffer + y * ls + x * ds;
					T *pi 	= image;

					if (ds == di && !sum)
					{
						// Faster option for images of equal depth when not summing
						int line = w * ds * sizeof(T);

						for (j=0; j<h; j++, pi+=li, ps+=ls)
						{
							memcpy(ps, pi, line);
						}
					}
					else
					{
						std::function<void(T *a, T *b)> convert = nullptr;

						if (sum)
						{
							if (ds == 3 && di == 3) convert = [](T *a, T *b) { b[0] += a[0]; b[1] += a[1]; b[2] += a[2]; };
							if (ds == 3 && di == 1) convert = [](T *a, T *b) { b[0] += a[0]; b[1] += a[0]; b[2] += a[0]; };
							if (ds == 1 && di == 3) convert = [](T *a, T *b) { b[0] += (a[0] + a[1] + a[2]) / 3; };
							if (ds == 1 && di == 1) convert = [](T *a, T *b) { b[0] += a[0]; };
						}
						else
						{
							if (ds == 3 && di == 1) convert = [](T *a, T *b) { b[0] = a[0]; b[1] = a[0]; b[2] = a[0]; };
							if (ds == 1 && di == 3) convert = [](T *a, T *b) { b[0] = (a[0] + a[1] + a[2]) / 3; };
						}

						if (convert)
						{
							int js = ls - w * ds;
							int ji = li - w * di;

							for (j=0; j<h; j++, pi+=ji, ps+=js)
							{
								for (i=0; i<w; i++, ps+=ds, pi+=di)
								{
									convert(pi, ps);
								}
							}
						}
					}
				}

				return *this;
			}


			/// Gets the value of a pixel for a specific channel but supports values outside the image dimensions,
			/// if mirror is true the pixel values are mirrored, otherwise they are smeared. Weird things will happen
			/// if a value that is twice the width/height outside the image is requested.
			T Value(int x, int y, byte channel = 0, bool mirror = true) const
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
			T Interpolate(double x, double y, byte channel = 0) const
			{
				if (x >= 0 && x < this->width - 1 && y >= 0 && y < this->height - 1 && channel < this->depth)
				{
					int line		= this->width * this->depth;
					double sx		= x - (int)x;
					double sy		= y - (int)y;
					T *pb			= this->buffer + (int)y * line + (int)x * this->depth + channel;
					double result	= (double)*pb * (1 - sx) * (1 - sy)
						+ (double)*(pb + this->depth) * sx * (1 - sy)
						+ (double)*(pb + line) * (1 - sx) * sy
						+ (double)*(pb + line + this->depth) * sx * sy;

					return std::is_integral<T>::value ? lrint(result) : result;
				}

				return 0;
			}


			/// Interpolate the pixel value of all channels at the given coordinates. The size
			/// N must match the depth of this image and the coordinates must be valid otherwise
			/// zeroes will be returned.
			template <byte N> std::array<T, N> InterpolateAll(double x, double y) const
			{
				std::array<T, N> result = {};

				if (N == this->depth && x >= 0 && x < this->width - 1 && y >= 0 && y < this->height - 1)
				{
					int line	= this->width * N;
					double sx	= x - (int)x;
					double sy	= y - (int)y;
					double a	= (1 - sx) * (1 - sy);
					double b	= sx * (1 - sy);
					double c	= (1 - sx) * sy;
					double d	= sx * sy;
					T *pb		= this->buffer + (int)y * line + (int)x * N;

					if (std::is_integral<T>::value)
					{
						for (int i=0; i<N; i++, pb++)
						{
							result[i] = lrint(a * *pb + b * pb[N]+ c * pb[line] + d * pb[line + N]);
						}
					}
					else
					{
						for (int i=0; i<N; i++, pb++)
						{
							result[i] = a * *pb + b * pb[N]+ c * pb[line] + d * pb[line + N];
						}
					}
				}

				return result;
			}



			/// Load an image from file. Uses the freeimage library to attempt loading and conversion
			/// of the image. It should cope with any standard image formats. If depth is 0 then the
			/// depth of this image will be left as it is, otherwise it will attempt to convert to the
			/// required depth where necessary.
			virtual bool Load(std::string path, byte depth = 0)
			{
				bool result = false;
				auto fif	= FreeImage_GetFileType(path.c_str(), 0);

				if (fif == FIF_UNKNOWN) fif = FreeImage_GetFIFFromFilename(path.c_str());

				if (fif != FIF_UNKNOWN && FreeImage_FIFSupportsReading(fif))
				{
					auto *image = FreeImage_Load(fif, path.c_str());

					if (image)
					{
						result = this->FromFib(image, depth);
						FreeImage_Unload(image);
					}
				}

				return result;
			}


			/// Load an image from memory buffer. If depth is 0 then the depth of this image will be left
			/// as it is, otherwise it will attempt to convert to the required depth where necessary.
			virtual bool Load(Buffer<byte> &buffer, byte depth = 0)
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
							result = this->FromFib(image, depth);
							FreeImage_Unload(image);
						}
					}

					FreeImage_CloseMemory(mem);
				}

				return result;
			}


			/// Load a raw image file, expects ImageHeader to be at the beginning
			virtual bool LoadRaw(std::string path)
			{
				return this->LoadRaw(path, false);
			}


			/// Save an image to file. Uses the freeimage library to attempt saving
			/// out the image (converts it to byte first but does not scale the values,
			/// so be warned). Image format is automatically determined by file extension.
			bool Save(std::string path, int compression = 0) const
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
			bool Save(Buffer<byte> &buffer, int compression) const
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


			/// Save a raw image file starting with ImageHeader
			bool SaveRaw(std::string path) const
			{
				if (this->Size())
				{
					std::ofstream ofs(path, std::ios::out | std::ios::binary);

					if (ofs.good())
					{
						ImageHeader header = { this->depth, sizeof(T), (uint16_t)this->width, (uint16_t)this->height };

						ofs.write((char *)&header, sizeof(ImageHeader));
						ofs.write((char *)this->Data(), this->width * this->height * this->depth * sizeof(T));
						ofs.flush();

						return true;
					}
				}

				return false;
			}


			typename Buffer<T>::iterator begin()				{ return this->buffer.begin(); }
			typename Buffer<T>::iterator end()					{ return this->buffer.end(); }
			typename Buffer<T>::const_iterator begin() const	{ return this->buffer.begin(); }
			typename Buffer<T>::const_iterator end() const		{ return this->buffer.end(); }


		protected:

			/// Image depth
			byte depth = 1;

			/// Image width
			int width = 0;

			/// Image height
			int height = 0;

			/// Buffer for the actual image data
			Buffer<T> buffer;



			template <typename U = T> typename std::enable_if<std::is_same<byte, U>::value, FIBITMAP *>::type ToFib() const
			{
				auto *result = FreeImage_ConvertFromRawBits(this->buffer, this->width, this->height, this->width * this->depth, this->depth * 8, 0, 0, 0, true);

				#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
					if (result && this->depth == 3)
					{
						int x, y;
						byte *data	= FreeImage_GetBits(result);
						int jump	= FreeImage_GetPitch(result) - this->width * this->depth;

						for (y=0; y<this->height; y++, data += jump)
						{
							for (x=0; x<this->width; x++, data += this->depth)
							{
								std::swap(data[0], data[2]);
							}
						}
					}
				#endif

				return result;
			}


			template <typename U = T> typename std::enable_if<!std::is_same<byte, U>::value, FIBITMAP *>::type ToFib() const
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
							*reinterpret_cast<uint16_t *>(dst) = *src;
							return sizeof(uint16_t);
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



			template <typename U = T> typename std::enable_if<std::is_same<byte, U>::value, bool>::type FromFib(FIBITMAP *image, byte depth)
			{
				this->width 	= FreeImage_GetWidth(image);
				this->height	= FreeImage_GetHeight(image);
				this->depth		= depth ? depth : this->depth;
				this->buffer.Resize(this->width * this->height * this->depth);

				FreeImage_ConvertToRawBits(this->buffer, image, this->width * this->depth, this->depth * 8, 0, 0, 0, true);

				#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
					if (this->depth == 3)
					{
						byte *end = this->buffer + this->width * this->height * this->depth;

						for (byte *data = this->buffer; data < end; data += this->depth)
						{
							std::swap(data[0], data[2]);
						}
					}
				#endif

				return true;
			}


			template <typename U = T> typename std::enable_if<!std::is_same<byte, U>::value, bool>::type FromFib(FIBITMAP *image, byte depth)
			{
				FIBITMAP *converted							= nullptr;
				std::function<int(byte *src, T *dst)> apply = nullptr;

				this->width 	= FreeImage_GetWidth(image);
				this->height	= FreeImage_GetHeight(image);
				this->depth		= depth ? depth : this->depth;
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
							*dst = *reinterpret_cast<uint16_t *>(src);
							return sizeof(uint16_t);
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


			/// A copy function that also attempts to do type conversion. For example, when going from an RGB int
			/// image to a greyscale byte image it will first convert from int to byte by normalising the values
			/// and scaling to byte (if necessary). Then it will convert from RGB to greyscale. If the source image
			/// is the same type as this then it should be pretty fast since it is a simple buffer copy instead.
			template <class U> void Copy(const ImageBase<U> &image)
			{
				const T max		= std::numeric_limits<T>::max();
				this->width		= image.width;
				this->height	= image.height;
				int size		= this->width * this->height;
				auto r			= image.buffer.Range();

				this->buffer.Resize(size * this->depth);

				U *src = image;
				T *dst = this->buffer;

				std::function<T(U value)> apply = nullptr;
				if (r.range > max)		apply = [&](U value) { return (T)lrint(max * r.normalise(value)); };
				else if (r.max > max)	apply = [&](U value) { return (T)(value - r.min); };
				else					apply = [&](U value) { return (T)value; };

				if (this->depth == image.depth)
				{
					size *= this->depth;
					for (int i=0; i<size; i++)
					{
						*dst++ = apply(*src++);
					}
				}
				else if (this->depth == 1 && image.depth == 3)
				{
					for (int i=0; i<size; i++, src+=3)
					{
						*dst++ = (apply(src[0]) + apply(src[1]) + apply(src[2])) / 3;
					}
				}
				else if (this->depth == 3 && image.depth == 1)
				{
					T value;
					for (int i=0; i<size; i++)
					{
						value = apply(*src++);
						*dst++ = value;
						*dst++ = value;
						*dst++ = value;
					}

				}
			}


			/// If the image to be copied is of the same type and depth as this one then simply copy the buffer, otherwise
			/// convert appropriately (supports grey to RGB and vice versa).
			void Copy(const ImageBase<T> &image)
			{
				this->width		= image.width;
				this->height	= image.height;

				if (this->depth != image.depth)
				{
					int size = this->width * this->height;
					this->buffer.Resize(size * this->depth);

					T *src = image;
					T *dst = this->buffer;

					if (this->depth == 1 && image.depth == 3)
					{
						for (int i=0; i<size; i++, src+=3) { *dst++ = (src[0] + src[1] + src[2]) / 3; }
					}
					else if (this->depth == 3 && image.depth == 1)
					{
						for (int i=0; i<size; i++, src++) { *dst++ = *src; *dst++ = *src; *dst++ = *src; }
					}

				}
				else this->buffer = image.buffer;
			}

			/// Load a raw image file, expects ImageHeader to be at the beginning
			bool LoadRaw(std::string path, bool checkDepth)
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

						if (checkDepth && h.depth != this->depth)
						{
							throw std::runtime_error(
								"Attempting to load an Image<> from raw as a different depth, "
								"if this is intentional please consider using an ImageBase<> instead"
							);
						}

						int length = h.width * h.height * h.depth * sizeof(T);

						if (h.typesize == sizeof(T) && size == sizeof(ImageHeader) + length)
						{
							this->Resize(h.width, h.height, h.depth);
							ifs.read((char *)this->Data(), size);

							return true;
						}
					}
				}

				return false;
			}


			/// Allows the private members of this class
			/// to be accessed by other template variants
			template<class U> friend class ImageBase;
	};
}
