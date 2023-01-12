#include "doctest.h"
#include <emergent/image/Image.hpp>

using emg::Image;
using emg::ImageBase;
using emg::byte;
using emg::rgb;


TEST_SUITE("image")
{
	TEST_CASE("constructing an image")
	{
		ImageBase<byte> src(3, 10, 10);
		src = 255;	// Assign 255 to all values in the image

		REQUIRE(src.Depth() == 3);
		REQUIRE(src.Size() == 100);


		SUBCASE("construct default byte image")
		{
			ImageBase<byte> img;

			CHECK(img.Depth() == 1);
			CHECK(img.Size() == 0);
		}

		SUBCASE("construct image by size")
		{
			ImageBase<byte> img(3, 10, 10);

			CHECK(img.Depth() == 3);
			CHECK(img.Size() == 100);
		}

		SUBCASE("construct image of larger integer type")
		{
			ImageBase<uint16_t> img(1, 10, 10);

			CHECK(img.Depth() == 1);
			CHECK(img.Size() == 100);
		}

		SUBCASE("construct image from another image")
		{
			ImageBase<byte> dst(src);

			CHECK(dst.Data() != src.Data());
			CHECK(dst.Depth() == 3);
			CHECK(dst.Size() == 100);
			CHECK(dst.Value(0, 0) == 255);
		}

		SUBCASE("construct image from another image of a different type")
		{
			ImageBase<uint16_t> dst(src);

			CHECK(dst.Depth() == 3);
			CHECK(dst.Size() == 100);
			CHECK(dst.Value(0, 0) == 255);
		}

		SUBCASE("construct fixed depth image from another image")
		{
			Image<byte, rgb> dst(src);

			CHECK(dst.Depth() == 3);
			CHECK(dst.Size() == 100);
			CHECK(dst.Value(0, 0, 1) == 255);
		}
	}



	TEST_CASE("assigning to an image")
	{
		ImageBase<byte> src(3, 10, 10);
		src = 255;	// Assign 255 to all values in the image

		SUBCASE("assign an image of the same type")
		{
			ImageBase<byte> dst;

			dst = src;

			CHECK(dst.Depth() == 3);
			CHECK(dst.Size() == 100);
			CHECK(dst.Value(0, 0) == 255);
		}

		SUBCASE("assign an image of a different type")
		{
			ImageBase<uint16_t> dst;

			dst = src;

			CHECK(dst.Depth() == 3);
			CHECK(dst.Size() == 100);
			CHECK(dst.Value(0, 0) == 255);
		}

		SUBCASE("assign a single value to the entire image")
		{
			ImageBase<byte> dst(3, 10, 10);

			dst = 128;

			CHECK(dst.IsBlank(128));
		}

		SUBCASE("clear an image (reset to all zeros)")
		{
			CHECK(src.Value(4, 4) == 255);

			src.Clear();

			CHECK(src.IsBlank());
		}
	}



	TEST_CASE("accessing raw data")
	{
		SUBCASE("an empty image provides a null pointer")
		{
			ImageBase<byte> test;
			CHECK((byte *)test == nullptr);
			CHECK(test.Data() == nullptr);
		}
	}

				/// Operator override to support implicit and explicit typecasting
				// operator T*()				{ return this->buffer.data(); }
				// operator const T*() const	{ return this->buffer.data(); }

				// /// Return the buffer data
				// T *Data()				{ return this->buffer.data(); }
				// const T *Data() const	{ return this->buffer.data(); }

				// /// Return the actual internal buffer.
				// /// WARNING: If you modify the size of this buffer you will corrupt the owner image
				// std::vector<T> &Internal() { return this->buffer; }



	TEST_CASE("property access")
	{
		// get image depth, width, height and size
	}
	// 			/// Return the image depth (in bytes)
	// 			byte Depth() const { return this->depth; }

	// 			/// Return the image width
	// 			int Width() const { return this->width; }

	// 			/// Return the image height
	// 			int Height() const { return this->height; }

	// 			/// Return the image size
	// 			int Size() const { return this->width * this->height; }



	TEST_CASE("resizing")
	{
		ImageBase<byte> src(3, 10, 10);

		// resize an image - keep depth/change depth


		SUBCASE("resizing to an invalid dimension will clear the image")
		{
			src.Resize(-10, -10);

			CHECK(src.Size() == 0);
		}


	}

	// 			/// Resizes the image buffers but destroys any existing image data.
	// 			/// A depth of 0 will keep the existing depth.
	// 			virtual void Resize(int width, int height, byte depth = 0)



	TEST_CASE("statistics")
	{
		// max / min value
		// count based on predicate
		// count zero values
		// check blank
		// calculate full stats with optional masking
	}
	// 			/// Returns the maximum value in the current image data (regardless of image depth).
	// 			T Max() const { return Operations::Max(this->buffer); }

	// 			/// Returns the minimum value in the current image data (regardless of image depth).
	// 			T Min() const { return Operations::Min(this->buffer); }

	// 			/// Count the number of values in the current image data (regardless of image depth)
	// 			/// that match the supplied predicate.
	// 			int Count(std::function<bool(T value)> predicate) const { return Operations::Count(this->buffer, predicate); }

	// 			/// Count the number of zero values in the current image data (regardless of image depth)
	// 			int ZeroCount() const { return Operations::ZeroCount(this->buffer); }

	// 			///Check if all the pixels in the image are set to the same value (regardless of image depth)
	// 			bool IsBlank(T reference = 0) const { return Operations::IsBlank(this->buffer, reference); }

	// 			/// Calculate the distribution statistics of the image mask can be optionally passed in;
	// 			/// zero values in the mask tell distribution to ignore the corresponding pixels in the image.
	// 			distribution Stats(Buffer<byte> *mask = nullptr) const


	TEST_CASE("modification")
	{
		// the OR/AND should be using const images for the modifiers...

		// clamp values within range
		// shift all values
		// apply a global threshold to the image
		// invert the image
		// apply arithmetic OR/AND via a modifier image
		// variance normalise the image
		// normalise the image data
		// truncate the image height
		// insert another image at a given position (optional summing)
	}

	// 			/// Clamp the current image data (regardless of image depth) to the supplied
	// 			/// lower and upper limits.
	// 			void Clamp(T lower, T upper) { Operations::Clamp(this->buffer, lower, upper); }

	// 			/// Shift all of the values in the image data (regardless of image depth) by the
	// 			/// specified amount.
	// 			void Shift(int value) { Operations::Shift(this->buffer, value); }

	// 			/// Threshold this image at the given value
	// 			void Threshold(T threshold, T high = 255, T low = 0) { Operations::Threshold(this->buffer, threshold, high, low); }

	// 			/// Inverts this image
	// 			void Invert() { Operations::Invert(this->buffer); }

	// 			/// Arimetic OR of this image with a modifier image. The images should be of the
	// 			/// same depth, but do not have to be as long as the underlying buffer sizes are
	// 			/// the same. If they are not then an exception will be thrown.
	// 			void OR(ImageBase<T> &modifier) { Operations::OR(this->buffer, modifier.buffer); }

	// 			/// Arimetic AND of this image with a modifier image. The images should be of the
	// 			/// same depth, but do not have to be as long as the underlying buffer sizes are
	// 			/// the same. If they are not then an exception will be thrown.
	// 			void AND(ImageBase<T> &modifier) { Operations::AND(this->buffer, modifier.buffer); }


	// 			/// Variance normalise the image to a target variance. Each channel is normalised independently
	// 			/// (although it assumes only greyscale and RGB).
	// 			bool VarianceNormalise(double targetVariance)

	// 			/// Normalise the image data. Each channel is normalised independently (although it assumes only
	// 			/// greyscale and RGB).
	// 			bool Normalise()

	// 			/// Truncate the height of the image down to the given height. Since this just CHECKs
	// 			/// a change of the height value and a cut of the buffer it allows you to chop the
	// 			/// bottom off of an image without needing to use an additional image buffer.
	// 			bool Truncate(int height)

	// 			/// Add the values from another image to this one at the given offset
	// 			/// Any values that drop off the edges are ignored. If sum is true then
	// 			/// values are summed into the destination. Only image depths of 3 and 1 are
	// 			/// supported unless sum is false and the depths are the same.
	// 			ImageBase<T> &Insert(const ImageBase<T> &image, int x, int y, bool sum = false)



	TEST_CASE("inspection")
	{
		// inspect a region of the image using the provided operation
		// retrieve value from specific coordinates and channel
		// interpolate pixel value at real coordinates
		// interpolate all channels at real coordinates
	}
	// 			// Apply an operation to inspect a given region of the image. The region must be fully
	// 			// contained within the image and if it is invalid then `operation` will not be invoked.
	// 			void Inspect(int rx, int ry, int rw, int rh, std::function<void(const T*)> operation) const

	// 			/// Gets the value of a pixel for a specific channel but supports values outside the image dimensions,
	// 			/// if mirror is true the pixel values are mirrored, otherwise they are smeared. Weird things will happen
	// 			/// if a value that is twice the width/height outside the image is requested.
	// 			T Value(int x, int y, byte channel = 0, bool mirror = true) const

	// 			/// Interpolate the pixel value of a specific channel at the given coordinates.
	// 			T Interpolate(double x, double y, byte channel = 0) const

	// 			/// Interpolate the pixel value of all channels at the given coordinates. The size
	// 			/// N must match the depth of this image and the coordinates must be valid otherwise
	// 			/// zeroes will be returned.
	// 			template <byte N> std::array<T, N> InterpolateAll(double x, double y) const


	TEST_CASE("i/o")
	{
		SUBCASE("construct image from path")
		{

		}
		// construct image from path
		// load image from file, default depth and force depth change
		// load image from buffer, default depth and force depth change
		// load raw image image
		// save image to path, compression override?
		// save image to buffer
		// save raw image file
	}
	// 			/// Load an image from file. Uses the freeimage library to attempt loading and conversion
	// 			/// of the image. It should cope with any standard image formats. If depth is 0 then the
	// 			/// depth of this image will be left as it is, otherwise it will attempt to convert to the
	// 			/// CHECKd depth where necessary.
	// 			virtual bool Load(std::string path, byte depth = 0)

	// 			/// Load an image from memory buffer. If depth is 0 then the depth of this image will be left
	// 			/// as it is, otherwise it will attempt to convert to the CHECKd depth where necessary.
	// 			virtual bool Load(Buffer<byte> &buffer, byte depth = 0)

	// 			/// Load a raw image file, expects ImageHeader to be at the beginning
	// 			virtual bool LoadRaw(std::string path)

	// 			/// Save an image to file. Uses the freeimage library to attempt saving
	// 			/// out the image (converts it to byte first but does not scale the values,
	// 			/// so be warned). Image format is automatically determined by file extension.
	// 			bool Save(std::string path, int compression = 0)

	// 			/// Save image to a memory buffer.
	// 			bool Save(Buffer<byte> &buffer, int compression)

	// 			/// Save a raw image file starting with ImageHeader
	// 			bool SaveRaw(std::string path)


	TEST_CASE("iteration")
	{

	}
				// image::Iterator<T> Row(const int y)

				// // Return a const iterator to a row in the image. It will automatically step between pixels
				// // and provide a pointer to the current position which gives access to all channels.
				// image::Iterator<const T> Row(const int y) const

				// // Return an iterator to a column in the image. It will automatically step between rows
				// // and provide a pointer to the current position which gives access to all channels.
				// image::Iterator<T> Column(const int x)

				// // Return a const iterator to a column in the image. It will automatically step between rows
				// // and provide a pointer to the current position which gives access to all channels.
				// image::Iterator<const T> Column(const int x) const

				// // Return an iterator to all of the pixels in the image. It will automatically step between
				// // pixels and provide a poitner to the current position which gives access to all channels.
				// image::Iterator<T> Pixels()

				// // Return a const iterator to all of the pixels in the image. It will automatically step between
				// // pixels and provide a poitner to the current position which gives access to all channels.
				// image::Iterator<const T> Pixels() const

				// auto begin()		{ return this->buffer.begin(); }
				// auto end()			{ return this->buffer.end(); }
				// auto begin() const	{ return this->buffer.begin(); }
				// auto end() const	{ return this->buffer.end(); }



	// 		protected:

	// 			/// Image depth
	// 			byte depth = 1;

	// 			/// Image width
	// 			int width = 0;

	// 			/// Image height
	// 			int height = 0;

	// 			/// Buffer for the actual image data
	// 			Buffer<T> buffer;



	// 			/// A copy function that also attempts to do type conversion. For example, when going from an RGB int
	// 			/// image to a greyscale byte image it will first convert from int to byte by normalising the values
	// 			/// and scaling to byte (if necessary). Then it will convert from RGB to greyscale. If the source image
	// 			/// is the same type as this then it should be pretty fast since it is a simple buffer copy instead.
	// 			template <class U> void Copy(const ImageBase<U> &image)

	// 			/// If the image to be copied is of the same type and depth as this one then simply copy the buffer, otherwise
	// 			/// convert appropriately (supports grey to RGB and vice versa).
	// 			void Copy(const ImageBase<T> &image)

	// 			/// Load a raw image file, expects ImageHeader to be at the beginning
	// 			bool LoadRaw(std::string path, bool checkDepth)
}
