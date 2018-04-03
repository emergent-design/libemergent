#pragma once

#include <emergent/image/ImageBase.hpp>


namespace emergent
{
	/// Templated image class supporting different types and depths.
	///
	/// Only numeric types of image are permitted, a compiler error
	/// will occur otherwise.
	template <class T = byte, byte D = 1> class Image : public ImageBase<T>
	{
		public:
			/// Default constructor
			Image() : ImageBase<T>(D) {}


			/// Constructor that allocates an image buffer of the required size
			Image(int width, int height) : ImageBase<T>(D, width, height) {}


			/// Constructor that loads an image from file
			Image(std::string path) : ImageBase<T>(path, D) {}


			/// Copy constructor with automatic type conversion
			template <class U> Image(const ImageBase<U> &image) : ImageBase<T>(D)
			{
				this->Copy(image);
			}


			/// Assignment override with type and depth conversion
			template <class U> Image<T, D> &operator=(const ImageBase<U> &image)
			{
				this->Copy(image);
				return *this;
			}


			/// Assignment override which will set all pixels in the image to the given value
			/// Can be used to clear all the pixels of an RGB image to 0,0,0 for example.
			Image<T, D> &operator=(const T value)
			{
				this->buffer = value;
				return *this;
			}


			/// Prevent the depth from being changed for this derived type of image.
			void Resize(int width, int height, byte depth = 0) override
			{
				if (depth && depth != D)
				{
					throw std::runtime_error(
						"Attempting to resize an Image<> to a different depth, "
						"if this is intentional please consider using an ImageBase<> instead"
					);
				}

				ImageBase<T>::Resize(width, height);
			}


			/// Prevent the depth from being changed for this derived type of image.
			bool Load(std::string path, byte depth = 0) override
			{
				if (depth && depth != D)
				{
					throw std::runtime_error(
						"Attempting to load an Image<> as a different depth, "
						"if this is intentional please consider using an ImageBase<> instead"
					);
				}

				return ImageBase<T>::Load(path);
			}


			/// Prevent the depth from being changed for this derived type of image.
			bool Load(Buffer<byte> &buffer, byte depth = 0) override
			{
				if (depth && depth != D)
				{
					throw std::runtime_error(
						"Attempting to load an Image<> as a different depth, "
						"if this is intentional please consider using an ImageBase<> instead"
					);
				}

				return ImageBase<T>::Load(buffer);
			}


			/// Prevent the depth from being changed for this derived type of image.
			bool LoadRaw(std::string path) override
			{
				return ImageBase<T>::LoadRaw(path, true);
			}
	};
}
