#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/thread/Pool.hpp>
#include <emergent/logger/Logger.hpp>

#include <memory>
#include <future>
#include <chrono>
#include <list>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>


namespace emergent {
namespace usock
{

	template <std::size_t THREADS = 0> class Server
	{
		public:


			void OnMessage(std::function<bool(const std::vector<byte>&, std::vector<byte>&)> &&handler)
			{
				this->onMessage = std::move(handler);
			}


			bool Initialise(const std::string path, int maxConnections = 1024, int backlog = 32)
			{
				std::lock_guard<std::mutex> lock(this->cs);

				this->Dispose();
				this->watchlist.assign(maxConnections + 1, { -1, 0, 0 });

				auto &primary	= this->watchlist[0];
				primary.fd		= socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
				primary.events	= POLLIN;

				if (primary.fd != -1)
				{
					sockaddr_un address	= { 0 };
					address.sun_family	= AF_UNIX;

					strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
					unlink(path.c_str());

					if (bind(primary.fd, (sockaddr *)&address, sizeof(address)) == 0)
					{
						if (listen(primary.fd, backlog) == 0)
						{
							Log::Info("usock::Server listening at %s", path);
							this->used = 1;
							return true;
						}
					}

					close(primary.fd);
					primary.fd = -1;
				}

				return false;
			}



			void Poll(int timeout)
			{
				this->cs.lock();
					int rc = poll(this->watchlist.data(), this->used, timeout);
				this->cs.unlock();

				if (rc < 0)
				{
					Log::Error("usock::Server poll error: %s", strerror(errno));
					return;
				}

				if (!rc) return;

				int primary = this->watchlist[0].fd;

				for (auto &w : this->watchlist)
				{
					if (w.fd < 0) continue;

					if (w.revents & POLLHUP)
					{
						this->Close(&w);
					}
					else if (w.revents & POLLIN)
					{
						if (w.fd == primary)
						{
							this->Add(accept4(primary, nullptr, nullptr, 0));
						}
						else if (THREADS)
						{
							// Ignore events for this client while it is being handled by a thread.
							w.events = 0;
							this->pool.Run(std::bind(&Server::Read, this, &w));
						}
						else
						{
							this->Read(&w);
						}


					}

					w.revents = 0;
				}
			}


			~Server()
			{
				this->Dispose();
			}


		private:

			void Dispose()
			{
				for (auto &w : this->watchlist)
				{
					if (w.fd >= 0) close(w.fd);
				}

				this->watchlist.clear();
			}


			void Add(int client)
			{
				std::lock_guard<std::mutex> lock(this->cs);

				if (client != -1)
				{
					for (auto &w : this->watchlist)
					{
						if (w.fd < 0)
						{
							Log::Info("usock::Server client %d connected", client);

							w = { client, POLLIN, 0 };
							this->Refresh();
							return;
						}
					}

					close(client);

					Log::Error("usock::Server could not handle client %d, no available connections", client);
				}
			}


			bool Close(pollfd *client, int error = 0)
			{
				std::lock_guard<std::mutex> lock(this->cs);

				close(client->fd);

				if (error)	Log::Error("usock::Server client %d disconnected due to error: %s", client->fd, strerror(error));
				else		Log::Info("usock::Server client %d disconnected", client->fd);

				client->fd = -1;
				this->Refresh();

				return false;
			}


			// Refresh the count of used items in the watchlist by finding
			// the last one that has been assigned
			void Refresh()
			{
				int size 	= this->watchlist.size();
				this->used	= 1;

				for (int i=size-1; i>0; i--)
				{
					if (this->watchlist[i].fd >= 0)
					{
						this->used = i + 1;
						return;
					}
				}
			}


			bool Read(pollfd *client)
			{
				static thread_local std::vector<byte> input(1024);
				static thread_local std::vector<byte> output(1024);

				uint32_t size	= 0;
				uint32_t total	= 0;

				int rc = read(client->fd, &size, sizeof(uint32_t));

				if (rc != sizeof(uint32_t))
				{
					return this->Close(client, rc < 0 ? errno : 0);
				}

				input.resize(size);

				while (total < size)
				{
					rc = read(client->fd, input.data() + total, size - total);

					if (rc < 1)
					{
						return this->Close(client, rc < 0 ? errno : 0);
					}
					else
					{
						total += rc;
					}
				}

				if (total && this->onMessage && this->onMessage(input, output))
				{
					this->Write(client, output);
				}

				client->events = POLLIN;
				return true;
			}


			void Write(pollfd *client, std::vector<byte> &buffer)
			{
				uint32_t size	= buffer.size();
				uint32_t total	= 0;


				int rc = write(client->fd, &size, sizeof(uint32_t));

				if (rc != sizeof(uint32_t))
				{
					return this->Close(client, rc < 0 ? errno : 0);
				}

				while (total < size)
				{
					rc = write(client->fd, buffer.data() + total, size - total);

					if (rc < 1)
					{
						return this->Close(client, rc < 0 ? errno : 0);
					}
					else
					{
						total += rc;
					}
				}
			}


			// Time to wait (ms) when polling during a read/write.
			static const int POLL_RW_WAIT = 100;

			std::function<bool(const std::vector<byte>&, std::vector<byte>&)> onMessage;

			std::vector<pollfd> watchlist;
			int used = 0;

			ThreadPool<THREADS> pool;
			std::mutex cs;
	};

}}
