#include "emergent/thread/ResetEvent.h"

using namespace std;
using namespace chrono;


void ResetEvent::Set()
{
	lock_guard<mutex> lock(this->cs);

	this->flag = true;
	this->condition.notify_one();
}


void ResetEvent::Reset()
{
	lock_guard<mutex> lock(this->cs);

	this->flag = false;
}


bool ResetEvent::Wait(int timeout, bool reset)
{
	unique_lock<mutex> lock(this->cs);

	if (!this->flag)
	{
		if (timeout > 0)
		{
			if (this->condition.wait_for(lock, milliseconds(timeout)) == cv_status::timeout)
			{
				return false;
			}
		}
		else this->condition.wait(lock);
	}

	if (reset) this->flag = false;

	return true;
}
