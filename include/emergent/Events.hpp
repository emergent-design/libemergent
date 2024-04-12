#pragma once

#include <emergent/logger/Logger.hpp>
#include <shared_mutex>
#include <functional>
#include <concepts>
#include <memory>
#include <deque>
#include <list>


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



	// A key-based events publisher.
	// Rather than dealing with events by type (see the PolymorphicPublisher) this has a single Event type (which can be a struct/class or even a primitive).
	// The type of Key can be any type which can be used as the key in a std::map<>.
	// Subscriptions are tied to the value of key and when an event is raised the key must be specified so that all relevant subscribers can be notified.
	// This construct can be useful for monitoring changes in a key-value store or for observing changes within a tree structure.
	template <typename Key, typename Event, size_t QUEUE = 1024> class KeyPublisher
	{
		public:

			using EventPtr	= std::shared_ptr<const Event>;
			using Callback	= std::function<void(EventPtr)>;
			using SubBase	= SubscriptionBase<Event, KeyPublisher, QUEUE>;


			class Subscription : public SubBase
			{
				public:

					explicit Subscription(const Key &key, KeyPublisher &publisher, Callback callback) : SubBase(publisher), callback(callback), key(key)
					{
						this->publisher.Attach(*this);
					}

					const Key &What() const
					{
						return this->key;
					}


				protected:

					void Publish(EventPtr event) override
					{
						this->callback(event);
					}


				private:

					Callback callback;
					Key key;
			};


			// Subscribe to a specific key - the subscription is bound to the lifetime of the return value so it will automatically
			// detach itself from the publisher when destroyed. Subscriptions should therefore not be permitted to live beyond the
			// scope of the publisher.
			[[nodiscard]] std::unique_ptr<Subscription> Subscribe(const Key &key, Callback callback)
			{
				// return Subscription { key, *this, callback };
				return std::make_unique<Subscription>(key, *this, callback);
			}


			// Raise an event. All subscribers that are registered for the specific key will be notified.
			bool Raise(const Key &key, EventPtr event)
			{
				std::shared_lock lock(this->cs);

				if (!this->subscribers.contains(key))
				{
					return false;
				}

				for (auto &s : this->subscribers[key])
				{
					s.get().Push(event);
				}

				return true;
			}


			bool Raise(const Key &key, const Event &event)
			{
				return this->Raise(key, std::make_shared<const Event>(event));
			}


		private:

			using Subscriber = std::reference_wrapper<SubBase>;


			// Attach the subscriber to a specific key
			void Attach(SubBase &sub)
			{
				std::unique_lock lock(this->cs);

				this->subscribers[static_cast<Subscription&>(sub).What()].push_front(sub);
			}


			void Detach(SubBase &sub)
			{
				std::unique_lock lock(this->cs);

				const auto &key = static_cast<Subscription &>(sub).What();

				if (this->subscribers.contains(key))
				{
					this->subscribers[key].remove_if([&](auto &s) {
						return &s.get() == &sub;
					});
				}
			}


			std::map<Key, std::list<Subscriber>> subscribers;
			std::shared_mutex cs;

			// The subscription must be friended so that it can access the Attach/Detach
			// functions that should not be used externally
			friend SubBase;
	};


	// An type-based event publisher.
	// The type Base must be the base class/struct for all events that will be used with this publisher.
	// All subscribers are placed in a single list and will selectively queue events based on type when they are raised.
	template <typename Base, size_t QUEUE = 1024> class PolymorphicPublisher
	{
		public:

			using EventPtr	= std::shared_ptr<const Base>;
			using SubBase	= SubscriptionBase<Base, PolymorphicPublisher, QUEUE>;


			// A subscript for a specific event type. Event must be derived from the base event class/struct.
			template <typename Event> requires(std::derived_from<Event, Base>) class Subscription : public SubBase
			{
				public:
					using Callback = std::function<void(std::shared_ptr<const Event>)>;

					explicit Subscription(PolymorphicPublisher &publisher, Callback callback) : SubBase(publisher), callback(callback)
					{
						this->publisher.Attach(*this);
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
					s.get().Push(event);
				}
			}


			template <typename Event> requires(std::derived_from<Event, Base>) void Raise(const Event &event)
			{
				return this->Raise(std::make_shared<const Event>(event));
			}


		private:

			void Attach(SubBase &sub)
			{
				std::unique_lock lock(this->cs);

				this->subscribers.push_front(sub);
			}


			void Detach(SubBase &sub)
			{
				std::unique_lock lock(this->cs);

				this->subscribers.remove_if([&](auto &s) {
						return &s.get() == &sub;
				});
			}


			std::list<std::reference_wrapper<SubBase>> subscribers;
			std::shared_mutex cs;

			// The subscription must be friended so that it can access the Attach/Detach
			// functions that should not be used externally
			friend SubBase;
	};
}
