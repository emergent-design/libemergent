#pragma once

#include <emergent/Maths.hpp>
#include <vector>


namespace emergent
{
	/// Utility structure for generically analysing numeric data
	struct distribution
	{
		double min		= 0;	///< Minimum value
		double max		= 0;	///< Maximum value
		double mean		= 0;	///< Mean value
		double variance	= 0;	///< Variance of data
		double samples	= 0;	///< Number of samples
		double sum		= 0;	///< Sum of values
		double squared	= 0;	///< Sum of squares


		/// Default constructor
		distribution() {}


		/// Constructor with automatic analysis of supplied data - requires that data is a contiguous container
		template <class T> distribution(const T &data, std::vector<byte> *mask = nullptr)
		{
			this->analyse(data, mask);
		}


		/// Constructor with automatic analysis of supplied data
		template <class T> distribution(T *data, int size, byte *mask = nullptr)
		{
			this->analyse(data, size, mask);
		}


		/// Generate stats from a container - requires that data is contiguous
		template <class T> bool analyse(const T &data, std::vector<byte> *mask = nullptr)
		{
			static_assert(is_contiguous<T>, "source must be a contiguous container type");

			if (mask && mask->size() != data.size())
			{
				return false;
			}

			return this->analyse(data.data(), data.size(), mask ? mask->data() : nullptr);
		}


		/// Generate the distribution stats
		/// If mask is used it MUST be the same size as data.
		template <class T> bool analyse(T *data, int size, byte *mask = nullptr)
		{
			if (!size) return false;

			double value	= (double)*data++;
			double sum		= value;
			double max		= value;
			double min		= value;
			double squared	= value * value;

			if (!mask)
			{
				for (int i=1; i<size; i++)
				{
					value		 = (double)*data++;
					sum			+= value;
					squared		+= value * value;
					if (value < min) min = value;
					if (value > max) max = value;
				}
			}
			else
			{
				int count = 0;

				for (int i=1; i<size; i++, mask++, data++)
				{
					if (*mask)
					{
						value		 = (double)*data;
						sum			+= value;
						squared		+= value * value;
						if (value < min) min = value;
						if (value > max) max = value;
						count++;
					}
				}
				size = count;
			}

			this->sum		= sum;
			this->squared	= squared;
			this->samples	= size;
			this->min		= min;
			this->max		= max;
			this->mean		= size ? sum / size : 0;
			this->variance	= size ? (squared / size) - (this->mean * this->mean) : 0;

			return true;
		}
	};
}
