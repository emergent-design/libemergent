#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/struct/Buffer.hpp>


namespace emergent
{
	// Helper functions to perform operations and queries on images and buffers
	class Operations
	{
		public:

			// Most of these can be deprecated in favour of std functions or
			// possible std::ranges in C++20

			/// Returns the maximum value in the data.
			// template <typename T> static T Max(const Buffer<T> &data)
			template <typename T> static T Max(const std::vector<T> &data)
			{
				return *std::max_element(data.begin(), data.end());
				// T result = std::numeric_limits<T>::min();
				// for (auto &d : data) if (d > result) result = d;

				// return result;
			}


			/// Returns the minimum value in the data.
			// template <typename T> static T Min(const Buffer<T> &data)
			template <typename T> static T Min(const std::vector<T> &data)
			{
				return *std::min_element(data.begin(), data.end());
				// T result = std::numeric_limits<T>::max();
				// for (auto &d : data) if (d < result) result = d;

				// return result;
			}


			/// Count the number of values in the data that match the supplied predicate.
			// template <typename T> static int Count(const Buffer<T> &data, std::function<bool(T value)> predicate)
			template <typename T> static int Count(const std::vector<T> &data, std::function<bool(T value)> predicate)
			{
				return std::count_if(data.begin(), data.end(), predicate);
				// int result = 0;
				// for (auto &d : data) if (predicate(d)) result++;

				// return result;
			}

			/// Count the number of zero values in the data
			// template <typename T> static int ZeroCount(const Buffer<T> &data)
			template <typename T> static int ZeroCount(const std::vector<T> &data)
			{
				return std::count_if(
					data.begin(), data.end(),
					[](auto v) { return !v; }
				);
				// int result = 0;
				// for (auto &d : data) if (!d) result++;

				// return result;
			}


			// template <typename T> static bool IsBlank(const Buffer<T> &data, const T reference = 0)
			template <typename T> static bool IsBlank(const std::vector<T> &data, const T reference = 0)
			{
				return !std::any_of(
					data.begin(), data.end(),
					[&](auto v) { return v != reference; }
				);
				// for (auto &d : data) if (d != reference) return false;

				// return true;
			}


			/// Clamp the data to the supplied lower and upper limits.
			// template <typename T> static void Clamp(Buffer<T> &data, T lower, T upper)
			template <typename T> static void Clamp(std::vector<T> &data, const T lower, const T upper)
			{
				for (auto &d : data)
				{
					#if __cpp_lib_clamp >= 201603L
						d = std::clamp(d, lower, upper);
					#else
						d = d < lower ? lower : (d > upper ? upper : d);
					#endif
				}
			}


			/// Threshold the data at the given value. If above or equal to the threshold a value will be set to
			/// "high" else it will be set to "low".
			// template <typename T> static void Threshold(Buffer<T> &data, T threshold, T high = 255, T low = 0)
			template <typename T> static void Threshold(std::vector<T> &data, const T threshold, const T high = 255, const T low = 0)
			{
				for (auto &d : data)
				{
					d = d < threshold ? low : high;
				}
			}


			/// Shift all of the values in the data by the specified amount.
			// template <typename T> static void Shift(Buffer<T> &data, const int value)
			template <typename T> static void Shift(std::vector<T> &data, const int value)
			{
				for (auto &d : data)
				{
					d = Maths::clamp<T>(d + value);
				}
			}


			/// Data inversion
			// template <typename T> static void Invert(Buffer<T> &data)
			template <typename T> static void Invert(std::vector<T> &data)
			{
				for (auto &d : data)
				{
					d = ~d;
				}
			}


			/// Arithmetic OR of data.
			// template<typename T> static void OR(Buffer<T> &result, Buffer<T> &modifier)
			template<typename T> static void OR(std::vector<T> &result, const std::vector<T> &modifier)
			{
				const size_t size = result.size();

				if (modifier.size() == size)
				{
					auto *dst = result.data();
					auto *src = modifier.data();

					for (size_t i=0; i<size; i++)
					{
						*dst++ |= *src++;
					}
				}
				else
				{
					throw std::runtime_error("To apply the OR operator buffer sizes must match");
				}
			}


			/// Arithmetic AND of data.
			// template<typename T> static void AND(Buffer<T> &result, Buffer<T> &modifier)
			template<typename T> static void AND(std::vector<T> &result, const std::vector<T> &modifier)
			{
				const size_t size = result.size();

				if (modifier.size() == size)
				{
					auto *dst = result.data();
					auto *src = modifier.data();

					for (int i=0; i<size; i++)
					{
						*dst++ &= *src++;
					}
				}
				else
				{
					throw std::runtime_error("To apply the AND operator buffer sizes must match");
				}
			}


			/// Variance normalise the data to a target variance. The data is assumed to be in
			/// consecutive channel order (RGB for example) and N indicates the depth which will
			/// result in each channel being normalised independently.
			// template <byte N, typename T> static bool VarianceNormalise(Buffer<T> &data, double targetVariance)
			template <byte N, typename T> static bool VarianceNormalise(std::vector<T> &data, double targetVariance)
			{
				static_assert(N > 0, "Number of channels must be greater than zero");

				const size_t total = data.size();

				if (total && total % N == 0)
				{
					T *d = data.data();
					double mean, variance;
					double sum[N] = {}, squared[N] = {}, scale[N], shift[N];

					const size_t size = total / N;

					for (size_t i=0; i<size; i++, d+=N)
					{
						for (byte j=0; j<N; j++)
						{
							squared[j]	+= d[j] * d[j];
							sum[j]		+= d[j];
						}
					}

					for (byte j=0; j<N; j++)
					{
						mean		= sum[j] / (double)size;
						variance	= (squared[j] / (double)size) - mean * mean;
						scale[j]	= sqrt(targetVariance / variance);
						shift[j]	= 128 - (mean * scale[j]);
					}

					d = data.data();

					for (size_t i=0; i<size; i++, d+=N)
					{
						for (byte j=0; j<N; j++)
						{
							d[j] = Maths::clamp<T>(lrint((double)d[j] * scale[j] + shift[j]));
						}
					}

					return true;
				}

				return false;
			}


			/// Normalise the data. The data is assumed to be in consecutive channel order (RGB
			/// for example) and N indicates the depth which will result in each channel being
			/// normalised independently.
			// template <byte N, typename T> static bool Normalise(Buffer<T> &data)
			template <byte N, typename T> static bool Normalise(std::vector<T> &data)
			{
				const size_t total = data.size();

				if (total && total % N == 0)
				{
					double scale[N];
					T low[N], high[N];

					T *d		= data.data();
					size_t size = total / N;

					std::fill_n(low, N, std::numeric_limits<T>::max());
					std::fill_n(high, N, std::numeric_limits<T>::min());

					for (size_t i=0; i<size; i++, d+=N)
					{
						for (byte j=0; j<N; j++)
						{
							low[j]	= std::min(low[j], d[j]);
							high[j]	= std::max(high[j], d[j]);
						}
					}

					for (byte j=0; j<N; j++)
					{
						scale[j] = 255.0 / std::max(1.0, (double)(high[j] - low[j]));
					}

					d = data.data();

					for (size_t i=0; i<size; i++, d+=N)
					{
						for (byte j=0; j<N; j++)
						{
							d[j] = lrint(scale[j] * (d[j] - low[j]));
						}
					}

					return true;
				}

				return false;
			}

	};
}


