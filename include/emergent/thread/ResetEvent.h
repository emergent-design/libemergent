#pragma once

#include <mutex>
#include <condition_variable>


namespace emergent
{
	/// Helper class that is the equivalent to both a ManualResetEvent and
	/// AutoResetEvent.
	class ResetEvent
	{
		public:

			/// Sets the state of the event, allowing a single thread to proceed.
			void Set();

			/// Resets the state of the event, a thread will block at the wait function.
			void Reset();

			/// Wait for the event, if it has already been set then the function will
			/// drop straight through, otherwise it will block until either Set is invoked
			/// or a timeout occurs.
			/// If "timeout" is greater than 0 the function will timeout if Set is not called
			/// in time and the function will return false.
			/// If "reset" is enabled then the flag will reset to false when Wait finishes (like
			/// an AutoResetEvent) otherwise the flag will be left alone (like a ManualResetEvent).
			bool Wait(int timeout = 0, bool reset = true);

		private:

			/// Stores the state of the event
			bool flag = false;

			/// The mutex used to protect this event
			std::mutex cs;

			/// Used to notify the Wait call so that it can unblock
			std::condition_variable condition;
	};
}
