#include <emergent/events/Subscription.hpp>
#include <shared_mutex>
#include <list>

namespace emergent::events
{
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


			virtual ~KeyPublisher() {}


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

}
