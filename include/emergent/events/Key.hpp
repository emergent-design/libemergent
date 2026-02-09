#pragma once

#include <emergent/events/Subscription.hpp>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>

namespace emergent::events
{
	// A key-based events publisher.
	// Rather than dealing with events by type (see the PolymorphicPublisher) this has a single Event type (which can be a struct/class or even a primitive).
	// The type of Key can be any type which can be used as the key in a std::map<>.
	// Subscriptions are tied to the value of key and when an event is raised the key must be specified so that all relevant subscribers can be notified.
	// This construct can be useful for monitoring changes in a key-value store or for observing changes within a tree structure.
	template <typename Key, typename Event> class KeyPublisher
	{
		public:

			using QueuePtr = std::shared_ptr<Queue<Event>>;


			// Subscribe to a specific key - when the subscription is destroyed the publisher will remove the
			// corresponding queue from the list the next time an event is raised.
			[[nodiscard]] std::unique_ptr<Subscription<Event>> Subscribe(const Key &key, typename Subscription<Event>::Callback callback, const size_t size = 1024)
			{
				std::unique_lock lock(this->cs);

				auto queue = std::make_shared<Queue<Event>>(size);

				this->subscribers[key].push_front(queue);

				return std::make_unique<Subscription<Event>>(queue, callback);
			}


			// Raise an event. All subscribers that are registered for the specific key will be notified.
			bool Raise(const Key &key, std::shared_ptr<const Event> event)
			{
				bool expired = false;	// flag to indicate if any subscribers are no longer listening

				{
					std::shared_lock lock(this->cs);

					if (!this->subscribers.contains(key))
					{
						return false;
					}

					for (auto &s : this->subscribers[key])
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

					this->subscribers[key].remove_if([](const auto &s) {
						return !s->listening;
					});
				}

				return true;
			}


			bool Raise(const Key &key, const Event &event)
			{
				return this->Raise(key, std::make_shared<const Event>(event));
			}


		private:

			std::map<Key, std::list<QueuePtr>> subscribers;
			std::shared_mutex cs;
	};

}
