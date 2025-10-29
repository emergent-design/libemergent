#include <emergent/events/Subscription.hpp>


namespace emergent::events
{
	// A message bus system where any client can post a message and it will
	// be received by all other clients.

	// template <typename Message, size_t QUEUE = 1024> class MessageBus
	// {
	// 	public:

	// 		using EventPtr	= std::shared_ptr<const Message>;
	// 		using SubBase	= SubscriptionBase<Message, MessageBus, QUEUE>;

	// 	private:
	// };
}
