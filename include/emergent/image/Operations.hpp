#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/struct/Buffer.hpp>


namespace emergent
{
	// Helper functions to perform operations and queries on images and buffers
	class Operations
	{
		public:

			/// Returns the maximum value in the data.
			template <typename T> static T Max(const Buffer<T> &data)
			{
				T result = std::numeric_limits<T>::min();
				for (auto &d : data) if (d > result) result = d;

				return result;
			}


			/// Returns the minimum value in the data.
			template <typename T> static T Min(const Buffer<T> &data)
			{
				T result = std::numeric_limits<T>::max();
				for (auto &d : data) if (d < result) result = d;

				return result;
			}


			/// Count the number of values in the data that match the supplied predicate.
			template <typename T> static int Count(const Buffer<T> &data, std::function<bool(T value)> predicate)
			{
				int result = 0;
				for (auto &d : data) if (predicate(d)) result++;

				return result;
			}

			/// Count the number of zero values in the data
			template <typename T> static int ZeroCount(const Buffer<T> &data)
			{
				int result = 0;
				for (auto &d : data) if (!d) result++;

				return result;
			}


			template <typename T> static bool IsBlank(const Buffer<T> &data, T reference = 0)
			{
				for (auto &d : data) if (d != reference) return false;

				return true;
			}


			/// Clamp the data to the supplied lower and upper limits.
			template <typename T> static void Clamp(Buffer<T> &data, T lower, T upper)
			{
				for (auto &d : data) d = d < lower ? lower : (d > upper ? upper : d);
			}


			/// Threshold the data at the given value. If above or equal to the threshold a value will be set to
			/// "high" else it will be set to "low".
			template <typename T> static void Threshold(Buffer<T> &data, T threshold, T high = 255, T low = 0)
			{
				for (auto &d : data) d = d < threshold ? low : high;
			}


			/// Shift all of the values in the data by the specified amount.
			template <typename T> static void Shift(Buffer<T> &data, int value)
			{
				for (auto &d : data) d = Maths::clamp<T>(d + value);
			}


			/// Data inversion
			template <typename T> static void Invert(Buffer<T> &data)
			{
				for (auto &d : data) d = ~d;
			}


			/// Arithmetic OR of data.
			template<typename T> static void OR(Buffer<T> &result, Buffer<T> &modifier)
			{
				int size = result.Size();

				if (modifier.Size() == size)
				{
					T *dst = result, *src = modifier;

					for (int i=0; i<size; i++) *dst++ |= *src++;
				}
				else throw std::runtime_error("To apply the OR operator buffer sizes must match");
			}


			/// Arithmetic AND of data.
			template<typename T> static void AND(Buffer<T> &result, Buffer<T> &modifier)
			{
				int size = result.Size();

				if (modifier.Size() == size)
				{
					T *dst = result, *src = modifier;

					for (int i=0; i<size; i++) *dst++ &= *src++;
				}
				else throw std::runtime_error("To apply the AND operator buffer sizes must match");
			}


			/// Variance normalise the data to a target variance. The data is assumed to be in
			/// consecutive channel order (RGB for example) and N indicates the depth which will
			/// result in each channel being normalised independently.
			template <byte N, typename T> static bool VarianceNormalise(Buffer<T> &data, double targetVariance)
			{
				static_assert(N > 0, "Number of channels must be greater than zero");

				int size = data.Size();

				if (size && size % N == 0)
				{
					T *d;
					int i, j;
					double mean, variance;
					double sum[N] = {}, squared[N] = {}, scale[N], shift[N];

					size /= N;

					for (i=0, d=data; i<size; i++)
					{
						for (j=0; j<N; j++)
						{
							squared[j]	+= *d * *d;
							sum[j]		+= *d++;
						}
					}

					for (i=0; i<N; i++)
					{
						mean		= sum[i] / (double)size;
						variance	= (squared[i] / (double)size) - mean * mean;
						scale[i]	= sqrt(targetVariance / variance);
						shift[i]	= 128 - (mean * scale[i]);
					}

					for (i=0, d=data; i<size; i++)
					{
						for (j=0; j<N; j++, d++)
						{
							*d = Maths::clamp<T>(lrint((double)*d * scale[j] + shift[j]));
						}
					}

					return true;
				}

				return false;
			}


			/// Normalise the data. The data is assumed to be in consecutive channel order (RGB
			/// for example) and N indicates the depth which will result in each channel being
			/// normalised independently.
			template <byte N, typename T> static bool Normalise(Buffer<T> &data)
			{
				int size = data.Size();

				if (size && size % N == 0)
				{
					T *d;
					int i, j;
					double scale[N];
					T low[N], high[N];

					size /= N;

					std::fill_n(low, N, std::numeric_limits<T>::max());
					std::fill_n(high, N, std::numeric_limits<T>::min());

					for (i=0, d=data; i<size; i++)
					{
						for (j=0; j<N; j++)
						{
							low[j]	= std::min(low[j], *d);
							high[j]	= std::max(high[j], *d++);
						}
					}

					for (i=0; i<N; i++)
					{
						scale[i] = 255.0 / std::max(1.0, (double)(high[i] - low[i]));
					}

					for (i=0, d=data; i<size; i++)
					{
						for (j=0; j<N; j++, d++)
						{
							*d = lrint(scale[j] * (*d - low[j]));
						}
					}

					return true;
				}

				return false;
			}

	};
}


