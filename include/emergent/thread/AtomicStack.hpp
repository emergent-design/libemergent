#pragma once
// #include <memory>
// #include <utility>
#include <atomic>
#include <optional>


namespace emergent::experimental
{
	template <typename T> class AtomicStack
	{
		public:
			AtomicStack() = default;
			AtomicStack(const AtomicStack &) = delete;
			AtomicStack &operator=(const AtomicStack &) = delete;


			void Push(const T &value)
			{
				this->Push(new Node(value));
			}

			void Push(T &&value)
			{
				this->Push(new Node(value));
			}


			std::optional<T> Pop()
			{
				Node *node = this->head.load(std::memory_order_relaxed);

				while (!this->head.compare_exchange_strong(node, node->next, std::memory_order_release, std::memory_order_relaxed))
				{
					// empty loop
				}

				if (node)
				{
					this->size--;

					// const auto value = std::move(node->data);
					const auto value = node->data;

					delete node;

					return value;
				}

				return std::nullopt;
			}


			size_t Size() const
			{
				return this->size;
			}


		private:

			struct Node
			{
				T data;

				Node *next = nullptr;

				Node(const T &data) : data(data) {}
				Node(T &&data) : data(std::move(data)) {}
			};

			std::atomic<Node *> head = nullptr;
			std::atomic<size_t> size = 0;


			void Push(Node *node)
			{
				node->next = this->head.load(std::memory_order_relaxed);

				while (!this->head.compare_exchange_strong(node->next, node, std::memory_order_release, std::memory_order_relaxed))
				{
					// empty loop
				}

				this->size++;
			}
	};
}
