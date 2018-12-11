#pragma once


namespace emergent
{
	// Latches true so that continued assignments will not set it back to false.
	// Can be explicitly set back to false with the Reset function.
	class Latch
	{
		public:

			Latch()	{}

			Latch(bool initial)
			{
				this->value = initial;
			}


			operator bool() const
			{
				return this->value;
			}


			Latch &operator=(bool value)
			{
				if (value)
				{
					this->value = true;
				}

				return *this;
			}


			void Reset()
			{
				this->value = false;
			}


		private:

			bool value = false;
	};
}
