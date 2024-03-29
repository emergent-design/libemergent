#pragma once

#include <algorithm>
#include <emergent/Emergent.hpp>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>


namespace emergent
{
	class Maths
	{

		public:

			/// Helper function used to clamp a value within the numeric range of the required type
			template <class T> static inline T clamp(long value)
			{
				return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
			}

			template <class T> static inline T clamp(unsigned long value)
			{
				return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
			}

			template <class T> static inline T clamp(int value)
			{
				return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : value;
			}

			template <class T> static inline T clamp(double value)
			{
				if (std::is_floating_point<T>::value)
				{
					return (value > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (value < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : (T)value;
				}
				else
				{
					long v = lrint(value);

					return (v > std::numeric_limits<T>::max()) ? std::numeric_limits<T>::max() : (v < std::numeric_limits<T>::min()) ? std::numeric_limits<T>::min() : (T)v;
				}
			}


			/// Lookup table for sine values where the index is the angle in degrees (0-359)
			static inline double sine(unsigned int angle)
			{
				static const std::array<double, 360> SINES {{
					 0,            0.017452406,  0.034899497,  0.052335956,  0.069756474,  0.087155743,  0.104528463,  0.121869343,  0.139173101,  0.156434465,
					 0.173648178,  0.190808995,  0.207911691,  0.224951054,  0.241921896,  0.258819045,  0.275637356,  0.292371705,  0.309016994,  0.325568154,
					 0.342020143,  0.358367950,  0.374606593,  0.390731128,  0.406736643,  0.422618262,  0.438371147,  0.453990500,  0.469471563,  0.484809620,
					 0.5,          0.515038075,  0.529919264,  0.544639035,  0.559192903,  0.573576436,  0.587785252,  0.601815023,  0.615661475,  0.629320391,
					 0.642787610,  0.656059029,  0.669130606,  0.681998360,  0.694658370,  0.707106781,  0.719339800,  0.731353702,  0.743144825,  0.754709580,
					 0.766044443,  0.777145961,  0.788010754,  0.798635510,  0.809016994,  0.819152044,  0.829037573,  0.838670568,  0.848048096,  0.857167301,
					 0.866025404,  0.874619707,  0.882947593,  0.891006524,  0.898794046,  0.906307787,  0.913545458,  0.920504853,  0.927183855,  0.933580426,
					 0.939692621,  0.945518576,  0.951056516,  0.956304756,  0.961261696,  0.965925826,  0.970295726,  0.974370065,  0.978147601,  0.981627183,
					 0.984807753,  0.987688341,  0.990268069,  0.992546152,  0.994521895,  0.996194698,  0.997564050,  0.998629535,  0.999390827,  0.999847695,
					 1,            0.999847695,  0.999390827,  0.998629535,  0.997564050,  0.996194698,  0.994521895,  0.992546152,  0.990268069,  0.987688341,
					 0.984807753,  0.981627183,  0.978147601,  0.974370065,  0.970295726,  0.965925826,  0.961261696,  0.956304756,  0.951056516,  0.945518576,
					 0.939692621,  0.933580426,  0.927183855,  0.920504853,  0.913545458,  0.906307787,  0.898794046,  0.891006524,  0.882947593,  0.874619707,
					 0.866025404,  0.857167301,  0.848048096,  0.838670568,  0.829037573,  0.819152044,  0.809016994,  0.798635510,  0.788010754,  0.777145961,
					 0.766044443,  0.754709580,  0.743144825,  0.731353702,  0.719339800,  0.707106781,  0.694658370,  0.681998360,  0.669130606,  0.656059029,
					 0.642787610,  0.629320391,  0.615661475,  0.601815023,  0.587785252,  0.573576436,  0.559192903,  0.544639035,  0.529919264,  0.515038075,
					 0.5,          0.484809620,  0.469471563,  0.453990500,  0.438371147,  0.422618262,  0.406736643,  0.390731128,  0.374606593,  0.358367950,
					 0.342020143,  0.325568154,  0.309016994,  0.292371705,  0.275637356,  0.258819045,  0.241921896,  0.224951054,  0.207911691,  0.190808995,
					 0.173648178,  0.156434465,  0.139173101,  0.121869343,  0.104528463,  0.087155743,  0.069756474,  0.052335956,  0.034899497,  0.017452406,
					 0,           -0.017452406, -0.034899497, -0.052335956, -0.069756474, -0.087155743, -0.104528463, -0.121869343, -0.139173101, -0.156434465,
					-0.173648178, -0.190808995, -0.207911691, -0.224951054, -0.241921896, -0.258819045, -0.275637356, -0.292371705, -0.309016994, -0.325568154,
					-0.342020143, -0.358367950, -0.374606593, -0.390731128, -0.406736643, -0.422618262, -0.438371147, -0.453990500, -0.469471563, -0.484809620,
					-0.5,         -0.515038075, -0.529919264, -0.544639035, -0.559192903, -0.573576436, -0.587785252, -0.601815023, -0.615661475, -0.629320391,
					-0.642787610, -0.656059029, -0.669130606, -0.681998360, -0.694658370, -0.707106781, -0.719339800, -0.731353702, -0.743144825, -0.754709580,
					-0.766044443, -0.777145961, -0.788010754, -0.798635510, -0.809016994, -0.819152044, -0.829037573, -0.838670568, -0.848048096, -0.857167301,
					-0.866025404, -0.874619707, -0.882947593, -0.891006524, -0.898794046, -0.906307787, -0.913545458, -0.920504853, -0.927183855, -0.933580426,
					-0.939692621, -0.945518576, -0.951056516, -0.956304756, -0.961261696, -0.965925826, -0.970295726, -0.974370065, -0.978147601, -0.981627183,
					-0.984807753, -0.987688341, -0.990268069, -0.992546152, -0.994521895, -0.996194698, -0.997564050, -0.998629535, -0.999390827, -0.999847695,
					-1,           -0.999847695, -0.999390827, -0.998629535, -0.997564050, -0.996194698, -0.994521895, -0.992546152, -0.990268069, -0.987688341,
					-0.984807753, -0.981627183, -0.978147601, -0.974370065, -0.970295726, -0.965925826, -0.961261696, -0.956304756, -0.951056516, -0.945518576,
					-0.939692621, -0.933580426, -0.927183855, -0.920504853, -0.913545458, -0.906307787, -0.898794046, -0.891006524, -0.882947593, -0.874619707,
					-0.866025404, -0.857167301, -0.848048096, -0.838670568, -0.829037573, -0.819152044, -0.809016994, -0.798635510, -0.788010754, -0.777145961,
					-0.766044443, -0.754709580, -0.743144825, -0.731353702, -0.719339800, -0.707106781, -0.694658370, -0.681998360, -0.669130606, -0.656059029,
					-0.642787610, -0.629320391, -0.615661475, -0.601815023, -0.587785252, -0.573576436, -0.559192903, -0.544639035, -0.529919264, -0.515038075,
					-0.5,         -0.484809620, -0.469471563, -0.453990500, -0.438371147, -0.422618262, -0.406736643, -0.390731128, -0.374606593, -0.358367950,
					-0.342020143, -0.325568154, -0.309016994, -0.292371705, -0.275637356, -0.258819045, -0.241921896, -0.224951054, -0.207911691, -0.190808995,
					-0.173648178, -0.156434465, -0.139173101, -0.121869343, -0.104528463, -0.087155743, -0.069756474, -0.052335956, -0.034899497, -0.017452406
				}};

				return SINES[angle % 360];
			}


			/// Lookup table for cosine values where the index is the angle in degrees (0-359)
			static inline double cosine(unsigned int angle)
			{
				return sine(angle + 90);
			}


			/// Normalised random number
			static inline double nrand()
			{
				return (double)rand() / (double)RAND_MAX;
			}


			// Return the closest even integer. If the supplied value is an odd integer then the
			// even value above it is returned.
			template <typename T> static constexpr long even(T value) noexcept
			{
				return (long)value + ((long)value % 2);
			}


			// Calculate the average value between two percentiles of a histogram.
			// The lower and upper parameters define the interpercentile range; valid values are
			// between 0.0 and 1.0 inclusive.
			template <std::size_t N> static double interpercentile(const std::array<int, N> &histogram, double lower, double upper)
			{
				if (lower >= upper || lower < 0.0 || upper > 1.0)
				{
					return 0.0;
				}

				long total		= 0;
				long count		= 0;
				long sum		= 0;
				long samples	= 0;

				for (auto &h : histogram) total += h;

				if (total)
				{
					long start	= lrint(lower * total);
					long end	= lrint(upper * total);

					for (int i=0; i<N; i++)
					{
						count += histogram[i];

						if (count >= end)
						{
							sum 	+= (end - start) * i;
							samples += end - start;
							break;
						}
						if (count > start)
						{
							sum 	+= (count - start) * i;
							samples += count - start;
							start	= count;
						}
					}

					return samples ? (double)sum / (double)samples : 0.0;
				}

				return 0.0;
			}


			// Find the median in a vector of numeric values. This will modify the
			// the supplied samples vector by partially sorting it.
			template <typename T> static T median(std::vector<T> &samples)
			{
				const auto size = samples.size();

				switch (size)
				{
					case 0:		return 0;
					case 1:		return samples[0];
					case 2:		return (samples[0] + samples[1]) / 2;
					default:	break;
				}

				const auto middle = size / 2;

				std::nth_element(samples.begin(), samples.begin() + middle, samples.end());

				return samples[middle];

				// Cannot take the mean of the two median values because nth_element
				// does not guarantee that the `middle - 1`th value is sorted.
				// return size % 2 ? samples[middle] : (samples[middle - 1] + samples[middle]) / 2;
			}


			// Find the floating-point mean of numeric values in a vector.
			template <typename T> static double mean(const std::vector<T> &samples)
			{
				return samples.size()
					? std::accumulate(samples.cbegin(), samples.cend(), 0.0) / (double)samples.size()
					: 0;
			}
	};


	template <> inline long Maths::clamp(long value)					{ return value; }
	template <> inline unsigned long Maths::clamp(unsigned long value)	{ return value; }
	template <> inline int Maths::clamp(int value)						{ return value; }
	template <> inline double Maths::clamp(double value)				{ return value; }
}

