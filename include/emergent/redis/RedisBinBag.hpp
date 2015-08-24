#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/image/Image.hpp>
#include <emergent/redis/Redis.hpp>


namespace emergent {
namespace redis
{
	struct ImageHeader
	{
		byte depth;
		byte typesize;
		uint16_t width;
		uint16_t height;
	};


	class RedisBinBag : public Redis
	{
		public:

			using Redis::Set;
			using Redis::Get;
			using Redis::Publish;


			template <typename T> bool Set(string key, Buffer<T> &buffer)
			{
				static_assert(std::is_arithmetic<T>::value, "Buffer type must be numeric");

				return Command("SET %s %b", key.c_str(), buffer.Data(), (size_t)(buffer.Size() * sizeof(T))).Ok();
			}


			template <typename T> bool Get(string key, Buffer<T> &buffer)
			{
				static_assert(std::is_arithmetic<T>::value, "Buffer type must be numeric");

				bool result = false;
				auto reply	= this->GetBinary(key);

				if (reply)
				{
					if (reply->len % sizeof(T) == 0)
					{
						buffer.Set((T *)reply->str, reply->len / sizeof(T));
						result = true;
					}
					else Log::Error("Attempt to retrieve buffer failed at '%s', type size mismatch", key);

					freeReplyObject(reply);
				}

				return result;
			}


			template <typename T> bool Set(string key, std::vector<T> &buffer)
			{
				static_assert(std::is_arithmetic<T>::value, "Vector type must be numeric");

				return Command("SET %s %b", key.c_str(), buffer.data(), (size_t)(buffer.size() * sizeof(T))).Ok();
			}


			template <typename T> bool Get(string key, std::vector<T> &buffer)
			{
				static_assert(std::is_arithmetic<T>::value, "Vector type must be numeric");

				bool result = false;
				auto reply	= this->GetBinary(key);

				if (reply)
				{
					if (reply->len % sizeof(T) == 0)
					{
						buffer.resize(reply->len / sizeof(T));
						memcpy(buffer.data(), reply->str, reply->len);
						result = true;
					}
					else Log::Error("Attempt to retrieve vector failed at '%s', type size mismatch", key);

					freeReplyObject(reply);
				}

				return result;
			}


			template <typename T> bool Set(string key, ImageBase<T> &image)
			{
				ImageHeader header = { image.Depth(), sizeof(T), (uint16_t)image.Width(), (uint16_t)image.Height() };

				return Command("SET %s %b%b", key.c_str(), &header, sizeof(ImageHeader), image.Data(), (size_t)(image.Size() * image.Depth() * sizeof(T))).Ok();
			}


			template <typename T> bool Get(string key, ImageBase<T> &image)
			{
				ImageHeader h;
				auto reply	= this->GetBinary(key);
				bool result	= false;

				if (reply && reply->len > sizeof(ImageHeader))
				{
					memcpy(&h, reply->str, sizeof(ImageHeader));

					int size = h.width * h.height * h.depth * sizeof(T);

					if (h.typesize == sizeof(T) && h.depth == image.Depth() && reply->len == sizeof(ImageHeader) + size)
					{
						image.Resize(h.width, h.height);
						memcpy(image.Data(), reply->str + sizeof(ImageHeader), size);
						result = true;
					}
					else Log::Error("Attempt to retrieve image failed at '%s', header does not match image metrics", key);
				}

				if (reply) freeReplyObject(reply);

				return result;
			}


			template <typename T> long Publish(string channel, ImageBase<T> &image)
			{
				ImageHeader header = { image.Depth(), sizeof(T), (uint16_t)image.Width(), (uint16_t)image.Height() };

				return Command("PUBLISH %s %b%b", channel.c_str(), &header, sizeof(ImageHeader), image.Data(), (size_t)(image.Size() * image.Depth() * sizeof(T))).AsLong();
			}


		protected:

			redisReply *GetBinary(string key)
			{
				auto reply = this->InvokeCommand("GET %s", key.c_str());

				if (reply)
				{
					if (reply->type == REDIS_REPLY_STRING)
					{
						return reply;
					}

					freeReplyObject(reply);
				}

				return nullptr;
			}
	};
}}

