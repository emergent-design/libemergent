#pragma once

#include <emergent/image/ImageBase.hpp>


namespace emergent
{
	/// Templated image class supporting different types and depths.
	///
	/// Only numeric types of image are permitted, a compiler error
	/// will occur otherwise.
	template <class T = byte, class D = Depth::Grey> class Image : public ImageBase<T>
	{
		public:
			/// Default constructor
			Image() : ImageBase<T>(D::type, D::size) {}


			/// Constructor that allocates an image buffer of the required size
			Image(int width, int height) : ImageBase<T>(D::type, D::size, width, height) {}


			/// Constructor that loads an image from file
			Image(std::string path) : ImageBase<T>(D::type, D::size)
			{
				this->Load(path);
			}


			/// Copy constructor with automatic type conversion
			template <class U, class E> Image(const Image<U, E> &image) : ImageBase<T>(D::type, D::size)
			{
				this->Copy((Image<U, E> *)&image);
			}


			/// Assignment override with type conversion
			template <class U, class E> Image<T, D>& operator=(const Image<U, E> &image)
			{
				this->Copy((Image<U, E> *)&image);
				return *this;
			}


			/// Assignment override which will set all pixels in the image to the given value
			/// Can be used to clear all the pixels of an RGB image to 0,0,0 for example.
			Image<T, D>& operator=(const T value)
			{
				this->buffer = value;
				return *this;
			}


			/// Assignment override which will set all pixels in the image to the given colour
			Image<T, D>& operator=(typename D::template Colour<T> &value)
			{
				int size	= this->width * this->height;
				T *dst		= this->buffer;
				for (int i=0; i<size; i++, dst+=D::size) value.Apply(dst);

				return *this;
			}


			/// Produces the inverse of this image.
			Image<T, D> operator~()
			{
				Image<T, D> result;

				return result ^= *this;
			}


			/// Assignment override for image inversion
			Image<T, D>& operator^=(Image<T, D> &image)
			{
				this->buffer.Resize(image.width * image.height * D::size);

				int size 		= this->buffer.Size();
				this->width		= image.width;
				this->height	= image.height;
				T *src			= image; //((Image<T, D>)image).buffer;
				T *dst			= this->buffer;

				for (int i=0; i<size; i++) *dst++ = ~*src++;

				return *this;
			}


			/// Assignment override for arithmetic OR with another image
			Image<T, D>& operator|=(Image<T, D> &image)
			{
				int size = this->buffer.Size();

				if (image.buffer.Size() == size)
				{
					T *src = image;
					T *dst = this->buffer;

					for (int i=0; i<size; i++) *dst++ |= *src++;
				}
				else throw std::runtime_error("To apply OR operator the image sizes must match");

				return *this;
			}


			/// Add the values from another image to this one at the given offset
			/// Any values that drop off the edges are ignored. If replace is true,
			/// the image is inserted rather than summed into the destination.
			Image<T, D>& Add(Image<T, D> &image, int x, int y, bool replace = false)
			{
				if (x >= 0 && x < this->width && y >= 0 && y < this->height)
				{
					int i, j, k;
					int line	= this->width * D::size;
					int w 		= std::min(image.Width(), this->width - x);
					int h 		= std::min(image.Height(), this->height - y);
					int js		= line - w * D::size;
					int ji		= image.Width() * D::size - w * D::size;
					T *ps 		= this->buffer + y * line + x * D::size;
					T *pi 		= image;

					if (replace)
					{
						for (j=0; j<h; j++, pi+=ji, ps+=js)
						{
							for (i=0; i<w; i++)
							{
								for (k=0; k<D::size; k++) *ps++ = *pi++;
							}
						}

					}
					else
					{
						for (j=0; j<h; j++, pi+=ji, ps+=js)
						{
							for (i=0; i<w; i++)
							{
								for (k=0; k<D::size; k++) *ps++ += *pi++;
							}
						}
					}
				}

				return *this;
			}


			/// Interpolate the pixel value of all channels at the
			/// given coordinates.
			typename D::template Colour<T> InterpolateAll(double x, double y)
			{
				if (x >= 0 && x < this->width - 1 && y >= 0 && y < this->height - 1)
				{
					int line	= this->width * D::size;
					double sx	= x - (int)x;
					double sy	= y - (int)y;
					double a	= (1 - sx) * (1 - sy);
					double b	= sx * (1 - sy);
					double c	= (1 - sx) * sy;
					double d	= sx * sy;
					T *pb		= this->buffer + (int)y * line + (int)x * D::size;

					T values[D::size];
					for (int i=0; i<D::size; i++, pb++) values[i] = lrint(a * *pb + b * pb[D::size]+ c * pb[line] + d * pb[line + D::size]);

					return typename D::template Colour<T>(values);
				}

				return typename D::template Colour<T>();
			}


		private:

			/// A copy function that also attempts to do type conversion.
			/// For example, when going from an RGB int image to a greyscale byte image it
			/// will first convert from int to byte by normalising the values and scaling to
			/// byte (if necessary). Then it will convert from RGB to greyscale. If the
			/// source image is the same type as this then it should be pretty fast since
			/// it is a simple buffer copy instead.
			template <class U, class E> void Copy(Image<U, E> *image)
			{
				this->width			= image->width;
				this->height		= image->height;
				Buffer<T> *buffer	= &this->buffer;

				if (typeid(U) != typeid(T))
				{
					int size = image->buffer.Size();
					if (D::type == E::type)	this->buffer.Resize(size);
					else					buffer = new Buffer<T>(size);

					bounds<U> r = image->buffer.Range();

					U *src	= *image;
					T *dst	= *buffer;
					T max	= std::numeric_limits<T>::max();

					if (r.range > max)	for (int i=0; i<size; i++) *dst++ = (T)lrint(max * r.normalise(*src++));
					else				for (int i=0; i<size; i++) *dst++ = (T)(*src++ - r.min);
				}
				else buffer = (Buffer<T> *)&image->buffer;


				if (D::type != E::type)
				{
					int size = this->width * this->height;
					this->buffer.Resize(size * D::size);

					T *src = *buffer;
					T *dst = this->buffer;

					switch (D::type)
					{
						case Depth::ID_GREY:	for (int i=0; i<size; i++) { E::ToGrey(src, dst);	src += E::size; dst++;		} break;
						case Depth::ID_RGB:		for (int i=0; i<size; i++) { E::ToRGB(src, dst);	src += E::size; dst += 3;	} break;
						case Depth::ID_RGBA:	for (int i=0; i<size; i++) { E::ToRGBA(src, dst);	src += E::size; dst += 4;	} break;
					}

					if (buffer != (Buffer<T> *)&image->buffer) delete buffer;
				}
			}


			/// If the image to be copied is of the same type and depth
			/// as this one, then simply copy the buffer.
			void Copy(Image<T, D> *image)
			{
				this->width		= image->width;
				this->height	= image->height;
				this->buffer	= image->buffer;
			}

			/// Allows the private members of this class
			/// to be accessed by other template variants
			template<class U, class E> friend class Image;
	};
}
