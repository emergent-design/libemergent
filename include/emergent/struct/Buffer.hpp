#pragma once

#include <emergent/Emergent.h>
#include <emergent/struct/Bounds.hpp>

#include <fstream>
#include <cstring>


namespace emergent
{
	/// Templated buffer implementation
	template <class T> class Buffer
	{
		public:

			/// Default constructor
			Buffer() {}


			/// Constructor with pre-allocation of memory
			Buffer(int size)
			{
				this->Resize(size);
			}


			/// Construct from a binary string
			Buffer(std::string data)
			{
				if (data.length() % sizeof(T) == 0)
				{
					this->Resize(data.length() / sizeof(T));
					memcpy(this->data, data.data(), data.length());
				}
				else throw "Attempting to construct a buffer from a binary string of invalid size";
			}


			/// Copy existing data
			Buffer(T *data, int size)
			{
				this->Set(data, size);
			}


			/// Copy constructor
			Buffer(const Buffer<T> &buffer)
			{
				this->data	= nullptr;
				this->size	= 0;
				this->max	= 0;
				this->Copy((Buffer<T> *)&buffer);
			}


			/// Construct from an initialiser list
			Buffer(std::initializer_list<T> data)
			{
				this->size	= data.size();
				this->max	= this->size;
				this->data	= new T[this->size];

				memcpy(this->data, data.begin(), this->size * sizeof(T));
			}


			/// Destructor
			~Buffer()
			{
				if (this->data) delete [] this->data;
			}


			/// Assignment operator override
			Buffer<T>& operator=(const Buffer<T> &buffer)
			{
				this->Copy((Buffer<T> *)&buffer);
				return *this;
			}


			/// Assignment operator override that will set the whole buffer to the given value
			Buffer<T>& operator=(const T value)
			{
				T *src = this->data;

				if (sizeof(T) > 1)	for (int i=0; i<this->size; i++) *src++ = value;
				else 				memset(src, value, this->size);

				return *this;
			}


			/// Assignment from a binary string
			Buffer<T>& operator=(const std::string value)
			{
				if (value.length() % sizeof(T) == 0)
				{
					this->Resize(value.length() / sizeof(T));
					memcpy(this->data, value.data(), value.length());
				}
				else throw "Attempting to assign a buffer from a binary string of invalid size";
			}


			/// Append the data from the supplied buffer to this buffer
			void Append(Buffer<T> &buffer)
			{
				if (this->max < this->size + buffer.Size())
				{
					T *temp		= this->data;
					this->max	= this->size + buffer.Size();
					this->data	= new T[this->max];

					memcpy(this->data, temp, this->size * sizeof(T));
					delete [] temp;
				}

				memcpy(this->data + this->size * sizeof(T), buffer.data, buffer.size * sizeof(T));

				this->size = size + buffer.size;
			}


			/// Copy existing data
			void Set(T *data, int size)
			{
				this->Resize(size);

				memcpy(this->data, data, size * sizeof(T));
			}


			/// Sets the data area to 0, so if T is numeric they will be set to 0, if T is a structure
			/// then the entire structure will be cleared.
			void Clear()
			{
				if (this->size) memset(this->data, 0, this->size * sizeof(T));
			}


			/// Resize the buffer. Will only actually re-allocate memory
			/// if size is greater than the storage capacity of the buffer.
			void Resize(int size)
			{
				if (size > this->max)
				{
					if (this->data) delete [] this->data;

					this->data	= new T[size];
					this->size	= size;
					this->max	= size;
				}
				else this->size = size;
			}


			/// Reduce the actual memory footprint of the buffer to the current size.
			void Compress()
			{
				if (this->max > size && size > 0)
				{
					T *temp		= this->data;
					this->data	= new T[size];
					this->size	= size;
					this->max	= size;

					memcpy(this->data, temp, this->size * sizeof(T));
					delete [] temp;
				}
			}


			/// Truncate the buffer to the given size. This allows you to crop a buffer
			/// down without requiring an additional buffer. (See image truncation).
			bool Truncate(int size)
			{
				if (size < this->size && size > 0)
				{
					this->size = size;
					return true;
				}

				return false;
			}


			/// Array style access operator overload
			T& operator [] (const int index)
			{
				return (index >= 0 && index < this->size) ? this->data[index] : this->dummy;
			}


			/// Return the current size of the buffer (not the storage capacity)
			int Size() const { return this->size; }

			/// Return the buffer data
			T *Data() const { return this->data; }

			/// Operator overload to support implicit and explicit typecasting
			operator T*() const { return this->data; }

			/// Create a binary string from the raw data
			operator std::string() {  return std::string((const char *)this->data, this->size * sizeof(T)); }


			/// Determine the range of values within the buffer (min/max/range).
			/// This is only valid for numeric types of buffer, if this function
			/// is called on any other type of buffer it will cause a compiler
			/// error.
			bounds<T> Range()
			{
				T min = 0, max = 0;

				if (this->size > 0)
				{
					T *src	= this->data;
					min		= max = *src++;

					for (int i=1; i<size; i++, src++)
					{
						if (*src < min) min = *src;
						if (*src > max) max = *src;
					}
				}

				return bounds<T>(min, max);
			}


			/// Calculates the sum of values within the buffer. This is
			/// only valid for numeric types of buffer.
			double Sum()
			{
				double sum = 0;

				if (this->size > 0)
				{
					T *src = this->data;

					for (int i=0; i<size; i++) sum += *src++;
				}

				return sum;
			}


			/// Save the buffer to binary file
			bool Save(std::string path)
			{
				std::ofstream ofs(path, std::ios::out | std::ios::binary);

				if (ofs.good())
				{
					ofs.put((char)sizeof(T));
					ofs.write((char *)this->data, this->size * sizeof(T));
					ofs.flush();

					return true;
				}

				return false;
			}


			/// Load the buffer from binary file
			bool Load(std::string path)
			{
				char typeSize;
				std::ifstream ifs(path, std::ios::in | std::ios::binary);

				if (ifs.good())
				{
					ifs.get(typeSize);

					if (typeSize == sizeof(T))
					{
						ifs.seekg(0, std::ios::end);
						int size = (int)ifs.tellg() - 1;
						ifs.seekg(1);

						if (size % sizeof(T) == 0)
						{
							this->Resize(size / sizeof(T));

							ifs.read((char *)this->data, size);

							return true;
						}
					}
				}

				return false;
			}


		private:

			/// Copy the supplied buffer data into this. It will
			/// only resize the internal buffer where necessary.
			void Copy(Buffer<T> *buffer)
			{
				int size = buffer->size;
				if (size > this->max)
				{
					if (this->data) delete [] this->data;

					this->data	= new T[size];
					this->max	= size;
				}

				this->size = size;
				memcpy(this->data, buffer->Data(), size * sizeof(T));
			}


			/// The actual buffer
			T *data = nullptr;

			/// Current size of the buffer (not the storage capacity)
			int size = 0;

			/// Storage capacity of the buffer
			int max = 0;

			/// Dummy value used by the array index operator overload
			/// when returning an out of bounds value reference.
			T dummy = T();
	};
}


#include <iomanip>

/// Allows a buffer to be passed to cout. If the buffer contains byte
/// data then it is output as hex.
template <class T> std::ostream &operator << (std::ostream &output, const emergent::Buffer<T> &b)
{
	if (std::is_same<T, byte>::value)
	{
		output << std::hex << std::setfill('0');
		for (int i=0; i<b.Size(); i++) output << std::setw(2) << (int)b[i] << " ";
		output << std::dec;
	}
	else
	{
		for (int i=0; i<b.Size(); i++) output << b[i] << " ";
	}

	return output << " (" << b.Size() << " bytes)";
}

