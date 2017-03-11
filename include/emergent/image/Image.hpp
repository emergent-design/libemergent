#pragma once

#include <emergent/image/ImageBase.hpp>


namespace emergent
{
	/// Templated image class supporting different types and depths.
	///
	/// Only numeric types of image are permitted, a compiler error
	/// will occur otherwise.
	template <typename T = byte, byte D = 1, class A = std::allocator<T>> class Image : public ImageBase<T, A>
	{
		public:
			/// Default constructor
			Image() : ImageBase<T, A>(D) {}


			/// Constructor that allocates an image buffer of the required size
			Image(int width, int height) : ImageBase<T, A>(D, width, height) {}


			/// Constructor that loads an image from file
			Image(std::string path) : ImageBase<T, A>(path, D) {}


			/// Copy constructor with automatic type conversion
			template <typename U, typename V> Image(const ImageBase<U, V> &image) : ImageBase<T, A>(D)
			{
				this->Copy(image);
			}


			/// Assignment override with type conversion
			template <typename U, typename V> Image<T, D, A> &operator=(const ImageBase<U, V> &image)
			{
				this->Copy(image);
				return *this;
			}


			/// Assignment override which will set all pixels in the image to the given value
			/// Can be used to clear all the pixels of an RGB image to 0,0,0 for example.
			Image<T, D, A> &operator=(const T value)
			{
				this->buffer = value;
				return *this;
			}


			/// Prevent the depth from being changed for this derived type of image.
			virtual void Resize(int width, int height, byte depth = 0)
			{
				if (depth && depth != D)
				{
					throw std::runtime_error(
						"Attempting to resize an Image<> to a different depth, "
						"if this is intentional please consider using an ImageBase<> instead"
					);
				}

				ImageBase<T, A>::Resize(width, height);
			}


			/// Prevent the depth from being changed for this derived type of image.
			virtual bool Load(std::string path, byte depth = 0)
			{
				if (depth && depth != D)
				{
					throw std::runtime_error(
						"Attempting to load an Image<> as a different depth, "
						"if this is intentional please consider using an ImageBase<> instead"
					);
				}

				return ImageBase<T, A>::Load(path);
			}


			/// Prevent the depth from being changed for this derived type of image.
			virtual bool Load(Buffer<byte> &buffer, byte depth = 0)
			{
				if (depth && depth != D)
				{
					throw std::runtime_error(
						"Attempting to load an Image<> as a different depth, "
						"if this is intentional please consider using an ImageBase<> instead"
					);
				}

				return ImageBase<T, A>::Load(buffer);
			}


			/// Prevent the depth from being changed for this derived type of image.
			virtual bool LoadRaw(std::string path)
			{
				return ImageBase<T, A>::LoadRaw(path, true);
			}
	};
}
