#pragma once

#include <emergent/struct/Buffer.h>
#include <emergent/Maths.h>
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
		distribution() 
		{	
		}
		
		
		/// Constructor with automatic analysis of supplied buffer
		template <class T> distribution(Buffer<T> &data)
		{
			this->analyse(data);
		}
		
		
		/// Constructor with automatic analysis of supplied data 
		template <class T> distribution(std::vector<T> &data)
		{
			this->analyse(data);
		}
		
		
		/// Constructor with automatic analysis of supplied data
		template <class T> distribution(T *data, int size)
		{
			this->analyse(data, size);
		}


		/// Generate stats from a buffer
		template <class T> bool analyse(Buffer<T> &data)
		{
			return this->analyse(data.Data(), data.Size());
		}
		
		
		/// Generate stats from a vector
		template <class T> bool analyse(std::vector<T> &data)
		{
			return this->analyse(&data.front(), data.size());
		}
		
		
		/// Generate the distribution stats
		template <class T> bool analyse(T* data, int size)
		{
			if (size > 0)
			{
				double value	= (double)*data++;
				double sum		= value;
				double max		= value;
				double min		= value;
				double squared	= value * value;

				for (int i=1; i<size; i++)
				{
					value		 = (double)*data++;
					sum			+= value;
					squared		+= value * value;
					if (value < min) min = value;
					if (value > max) max = value;
				}
		
				this->sum		= sum;
				this->squared	= squared;
				this->samples	= size;
				this->min		= min;
				this->max		= max;
				this->mean		= sum / size;
				this->variance	= (squared / size) - (this->mean * this->mean);
				return true;
			}
			
			return false;
		}
	};
}
