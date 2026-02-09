#pragma once

#include <emergent/logger/Logger.hpp>
#include <deque>
#include <memory>


namespace emergent::events
{
	// A shared queue that belongs to the publisher and subscriber.  When the subscriber is
	// destroyed the "listening" flag will be set to false so that the publisher can clean up.
	template <typename Event> class Queue
	{
		public:

			using EventPtr = std::shared_ptr<const Event>;


			Queue(const size_t size = 1024) : size(size) {}


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


			// Push a new event from the publisher.
			// If the queue is full then new events are dropped and a message is logged.
			bool Push(EventPtr event)
			{
				if (!this->listening)
				{
					return false;
				}

				std::lock_guard lock(this->cs);

				if (this->pending.size() < this->size)
				{
					this->full = false;
					this->pending.push_back(event);
				}
				else if (!this->full)
				{
					this->full = true;
					emergent::Log::Error("subscription: event queue for subscription is full - dropping new events");
				}

				return true;
			}


			// Set to false when the subscriber is destroyed
			std::atomic<bool> listening	= true;

		private:

			const size_t size			= 1024;
			bool full					= false;

			std::deque<EventPtr> pending;
			std::mutex cs;
	};


	// A shared Subscription class for the different publishers.
	// The Event type can be anything for a KeyPublisher but must be a struct/class for the Polymorphic publisher.
	template <typename Base, typename Event = Base> class Subscription
	{
		public:

			using QueuePtr	= std::shared_ptr<Queue<Base>>;
			using Callback	= std::function<void(std::shared_ptr<const Event>)>;


			~Subscription()
			{
				// Inform the queue that we are no longer listening
				this->queue->listening = false;
			}


			// See if there are any events on the queue for this subscription and invoke the callback.
			// This ensures that the callbacks are invoked on the same thread as the listener. If the
			// owner of this subscription fails to call Listen on a regular basis then the queue will
			// fill and new events will be lost.
			void Listen()
			{
				while (auto event = this->queue->Pop())
				{
					// Check that this event is interesting to this subscriber
					if (auto cast = std::dynamic_pointer_cast<const Event>(event))
					{
						this->callback(cast);
					}
				}
			}


			Subscription &operator=(Subscription &&) = default;

			explicit Subscription(QueuePtr &queue, Callback callback) : queue(queue), callback(callback) {}


		protected:

			// No copying allowed
			Subscription(const Subscription &)				= delete;
			Subscription &operator=(const Subscription &)	= delete;


		private:

			QueuePtr queue;
			Callback callback;
	};
}
