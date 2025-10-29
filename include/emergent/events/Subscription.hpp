#include <emergent/logger/Logger.hpp>
#include <deque>


namespace emergent::events
{
	// A shared Subscription base class for the different publishers.
	// The Event type can be anything for a KeyPublisher but must be a struct/class for the Polymorphic publisher.
	// The QUEUE template value limits the size of the queue in case a subscriber is not invoking the Listen() function on a regular basis.
	template <typename Event, typename Publisher, size_t QUEUE = 1024> class SubscriptionBase
	{
		public:
			using EventPtr = std::shared_ptr<const Event>;


			virtual ~SubscriptionBase()
			{
				this->publisher.Detach(*this);
			}

			// See if there are any events on the queue for this subscription and invoke the callback.
			// This ensures that the callbacks are invoked on the same thread as the listener. If the
			// owner of this subscription fails to call Listen on a regular basis then the queue will
			// fill and new events will be lost.
			void Listen()
			{
				while (auto e = this->Pop())
				{
					this->Publish(e);
				}
			}


			// Push a new event from the publisher. If the event is of interest to this subscriber then
			// add it to the queue so that it can be handled on the same thread as the listener.
			// If the queue is full then new events are dropped and a message is logged.
			void Push(EventPtr event)
			{
				if (this->Invalid(event))
				{
					return;
				}

				std::lock_guard lock(this->cs);

				if (this->pending.size() < QUEUE)
				{
					this->full = false;
					this->pending.push_back(event);
				}
				else if (!this->full)
				{
					this->full = true;
					emergent::Log::Error("subscription: event queue for subscription is full - dropping new events");
				}
			}

			SubscriptionBase &operator=(SubscriptionBase &&) = default;


		protected:

			// The derived subscription class must call publisher.Attach(...) in its constructor
			explicit SubscriptionBase(Publisher &publisher) : publisher(publisher) {}

			// No copying allowed
			SubscriptionBase(const SubscriptionBase &)				= delete;
			SubscriptionBase &operator=(const SubscriptionBase &)	= delete;

			// Override this to ensure that only specific events are queued
			virtual bool Invalid(EventPtr) { return false; }

			// Must be overridden to send the event to the listener.
			virtual void Publish(EventPtr) = 0;

			Publisher &publisher;

		private:

			// Pop an item off the queue if available
			EventPtr Pop()
			{
				std::lock_guard lock(this->cs);

				if (!this->pending.empty())
				{
					auto result = this->pending.front();
					this->pending.pop_front();

					return result;
				}

				return nullptr;
			}


			bool full = false;
			std::deque<EventPtr> pending;
			std::mutex cs;
	};
}
