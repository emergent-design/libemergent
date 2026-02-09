#pragma once

#include <emergent/events/Subscription.hpp>
#include <memory>
#include <shared_mutex>
#include <list>


namespace emergent::events
{
	// An type-based event publisher.
	// The type Base must be the base class/struct for all events that will be used with this publisher.
	// All subscriber queues are placed in a single list, events will be published to each, and when events
	// are popped off the queue their type is checked to see if they are off interest to that particular subscriber.
	template <typename Base> class PolymorphicPublisher
	{
		public:


			// Subscribe to a specific event type - when the subscription is destroyed the publisher will remove the
			// corresponding queue from the list the next time an event is raised.
			// Subscribing to the Base event class will allow a listener to receive all events.
			template <typename Event> requires(std::derived_from<Event, Base>) [[nodiscard]]
			std::unique_ptr<Subscription<Base, Event>> Subscribe(typename Subscription<Base, Event>::Callback callback, const size_t size = 1024)
			{
				std::unique_lock lock(this->cs);

				auto queue = std::make_shared<Queue<Base>>(size);

				this->subscribers.push_front(queue);

				return std::make_unique<Subscription<Base, Event>>(queue, callback);
			}


			// Raise an event - all subscribers that are able to handle the event type will be notified
			// next time they listen
			void Raise(std::shared_ptr<const Base> event)
			{
				// flag to indicate if any subscribers are no longer listening
				bool expired = false;

				{
					std::shared_lock lock(this->cs);

					for (auto &s : this->subscribers)
					{
						if (!s->Push(event))
						{
							expired = true;
						}
					}
				}


				if (expired)
				{
					// We have at least one subscriber that is no longer listening so
					// remove the corresponding queue from the list.
					std::unique_lock lock(this->cs);

					this->subscribers.remove_if([](const auto &s) {
						return !s->listening;
					});
				}
			}


			template <typename Event> requires(std::derived_from<Event, Base>) void Raise(const Event &event)
			{
				return this->Raise(std::make_shared<const Event>(event));
			}


		private:

			std::list<std::shared_ptr<Queue<Base>>> subscribers;
			std::shared_mutex cs;
	};
}
