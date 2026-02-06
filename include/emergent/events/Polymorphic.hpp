#pragma once

#include <emergent/events/Subscription.hpp>
#include <shared_mutex>
#include <list>


namespace emergent::events
{
	// An type-based event publisher.
	// The type Base must be the base class/struct for all events that will be used with this publisher.
	// All subscribers are placed in a single list and will selectively queue events based on type when they are raised.
	template <typename Base, size_t QUEUE = 1024> class PolymorphicPublisher
	{
		public:

			using EventPtr	= std::shared_ptr<const Base>;
			using SubBase	= SubscriptionBase<Base, QUEUE>;


			// A subscript for a specific event type. Event must be derived from the base event class/struct.
			template <typename Event> requires(std::derived_from<Event, Base>) class Subscription : public SubBase
			{
				public:
					using Callback = std::function<void(std::shared_ptr<const Event>)>;

					explicit Subscription(PolymorphicPublisher &publisher, Callback callback)
						: SubBase([this, &publisher] {
							publisher.Detach(this);
						}),
						callback(callback)
					{
						publisher.Attach(this);
					}


				protected:

					// The event is not of interest to this subscription if it cannot be dynamically cast to Event.
					bool Invalid(EventPtr event) override
					{
						return std::dynamic_pointer_cast<const Event>(event) == nullptr;
					}

					void Publish(EventPtr event) override
					{
						this->callback(std::dynamic_pointer_cast<const Event>(event));
					}

				private:

					Callback callback;
			};


			virtual ~PolymorphicPublisher() {}


			// Subscribe to a specific event type - the subscription is bound to the lifetime of the return value so it will automatically
			// detach itself from the publisher when destroyed. Subscriptions should therefore not be permitted to live beyond the
			// scope of the publisher.
			// Subscribing to the Base event class will allow a listener to receive all events.
			template <typename Event> requires(std::derived_from<Event, Base>) [[nodiscard]] std::unique_ptr<Subscription<Event>> Subscribe(typename Subscription<Event>::Callback callback)
			{
				// return Subscription<Event> { *this, callback };
				return std::make_unique<Subscription<Event>>(*this, callback);
			}

			// Raise an event - all subscribers that are able to handle the event type will be notified
			void Raise(EventPtr event)
			{
				std::shared_lock lock(this->cs);

				for (auto &s : this->subscribers)
				{
					// s.get().Push(event);
					s->Push(event);
				}
			}


			template <typename Event> requires(std::derived_from<Event, Base>) void Raise(const Event &event)
			{
				return this->Raise(std::make_shared<const Event>(event));
			}


		private:

			void Attach(SubBase *sub)
			{
				std::unique_lock lock(this->cs);

				this->subscribers.push_front(sub);
			}


			void Detach(SubBase *sub)
			{
				std::unique_lock lock(this->cs);

				this->subscribers.remove_if([&](auto &s) {
					return s == sub;
				});
			}


			std::list<SubBase *> subscribers;
			std::shared_mutex cs;

			// The subscription must be friended so that it can access the Attach/Detach
			// functions that should not be used externally
			friend SubBase;
	};
}
