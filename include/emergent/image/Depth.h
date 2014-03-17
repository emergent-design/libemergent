#pragma once


namespace emergent
{
	/// Container class for structures representing image depth.
	///
	/// The structures within this class provide information
	/// about the type, size and also functions that can be
	/// used to convert between depths.
	class Depth
	{
		public:
			/// Depth identifiers used internally by the Image class
			enum ImageDepth
			{
				ID_GREY,
				ID_RGB,
				ID_RGBA
			};

			// Update this if we ever start using a depth larger than RGBA!
			static const int max = 4;


			template <class T> struct Colour
			{
				virtual ~Colour() {}
				
				/// Apply this colour at the given address
				virtual void Apply(T *src) = 0;

				/// Blend this colour with the pixel value at the given
				/// address using the required alpha.
				virtual void Blend(T *src, double alpha) = 0;
			};
			

			/// Structure representing greyscale (8-bit)
			struct Grey
			{
				static const int type = ID_GREY;
				static const int size = 1;

				template <class T> static inline void ToGrey(T *src, T *dst)	{ *dst = *src; }
				template <class T> static inline void ToRGB(T *src, T *dst) 	{ *dst++ = *src; *dst++ = *src; *dst = *src; }
				template <class T> static inline void ToRGBA(T *src, T *dst)	{ *dst++ = *src; *dst++ = *src; *dst++ = *src; *dst = 255; }

				template <class T> struct Colour : public Depth::Colour<T>
				{
					T intensity;

					Colour()			: intensity(0)			{}
					Colour(T intensity) : intensity(intensity) 	{}
					Colour(T *values)	: intensity(*values)	{}
					void Apply(T *src)							{ *src = this->intensity; }
					void Blend(T *src, double alpha) 			{ *src = (T)((double)*src * (1-alpha) + (double)this->intensity * alpha); }

				};
			};


			/// Structure representing RGB (24-bit)
			struct RGB
			{
				static const int type = ID_RGB;
				static const int size = 3;

				template <class T> static inline void ToGrey(T *src, T *dst)	{ long v = *src++; v += *src++; v += *src; *dst = (T)(v / 3); }
				template <class T> static inline void ToRGB(T *src, T *dst) 	{ *dst++ = *src++; *dst++ = *src++; *dst = *src; }
				template <class T> static inline void ToRGBA(T *src, T *dst)	{ *dst++ = *src++; *dst++ = *src++; *dst++ = *src; *dst = 255; }

				template <class T> struct Colour : Depth::Colour<T>
				{
					T colour[3];

					Colour(T red, T green, T blue )		{ this->colour[0] = red; this->colour[1] = green; this->colour[2] = blue; }
					Colour() : colour { 0, 0, 0 }		{}
					Colour(T *values)					{ for (int i=0; i<3; i++) this->colour[i] = values[i]; }
					void Apply(T *src) 					{ memcpy(src, this->colour, 3 * sizeof(T)); }
					void Blend(T *src, double alpha) 	{ for (int i=0; i<3; i++, src++) *src = (T)((double)*src * (1-alpha) + (double)this->colour[i] * alpha); }
				};
			};


			/// Structure representing RGB with alpha layer (32-bit)
			struct RGBA
			{
				static const int type = ID_RGBA;
				static const int size = 4;

				template <class T> static inline void ToGrey(T *src, T *dst)	{ long v = *src++; v += *src++; v += *src; *dst = (T)(v / 3); }
				template <class T> static inline void ToRGB(T *src, T *dst) 	{ *dst++ = *src++; *dst++ = *src++; *dst = *src; }
				template <class T> static inline void ToRGBA(T *src, T *dst)	{ *dst++ = *src++; *dst++ = *src++; *dst++ = *src++; *dst = *src; }

				template <class T> struct Colour : Depth::Colour<T>
				{
					T colour[4];

					Colour(T red, T green, T blue, T alpha )	{ this->colour[0] = red; this->colour[1] = green; this->colour[2] = blue; this->colour[3] = alpha; }
					Colour() : colour { 0, 0, 0, 0 }			{}
					Colour(T *values)							{ for (int i=0; i<4; i++) this->colour[i] = values[i]; }
					void Apply(T *src) 							{ memcpy(src, this->colour, 4 * sizeof(T)); }
					void Blend(T *src, double alpha) 			{ for (int i=0; i<4; i++, src++) *src = (T)((double)*src * (1-alpha) + (double)this->colour[i] * alpha); }
				};
			};
	};

	// Common use case
	typedef Depth::RGB rgb;
}