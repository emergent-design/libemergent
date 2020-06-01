#pragma once

#include <emergent/Emergent.hpp>
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
	// A multi-threaded unix socket implementation. Each connection is assumed to be
	// long-lived and so threads are created when a connection is established.
	template <std::size_t CONNECTIONS = 64> class Server
	{
		public:

			Server()
			{
				this->run = false;
			}


			void OnMessage(std::function<bool(const std::string&, std::string&)> &&handler)
			{
				this->onMessage = std::move(handler);
			}


			bool Initialise(const std::string path, int backlog = 32)
			{
				if (this->run)
				{
					// Should only initialise once
					return false;
				}

				this->watchlist.fill({ -1, 0, 0 });

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

							this->used			= 1;
							this->run			= true;
							this->threads[0]	= std::thread(&Server::Entry, this);

							return true;
						}
						else Log::Error("usock::Server failed to initiate socket listen");
					}
					else Log::Error("usock::Server failed to bind socket");

					close(primary.fd);
					primary.fd = -1;
				}
				else Log::Error("usock::Server failed to open socket");

				return false;
			}


			~Server()
			{
				this->run = false;

				if (this->threads[0].joinable())
				{
					this->threads[0].join();
				}

				Log::Info("usock::Server exited");
			}


		private:

			void Entry()
			{
				const int primary = this->watchlist[0].fd;

				while (this->run)
				{
					this->cs.lock();
						int rc = poll(this->watchlist.data(), this->used, 1);
					this->cs.unlock();

					if (rc < 0)
					{
						Log::Error("usock::Server poll error: %s", strerror(errno));
						this->run = false;
					}
					else if (rc)
					{
						for (auto &w : this->watchlist)
						{
							if (w.fd < 0) continue;

							if (w.revents & POLLHUP)
							{
								this->Close(&w);
							}
							else if (w.revents & POLLIN && w.fd == primary)
							{
								this->Add(
									accept(primary, nullptr, nullptr)
								);
							}

							w.revents = 0;
						}
					}
				}

				// Close any remaining connections and associated threads
				for (int i=1; i<CONNECTIONS; i++)
				{
					if (this->watchlist[i].fd >= 0)
					{
						close(this->watchlist[i].fd);
						this->watchlist[i].fd = -1;
					}
					if (this->threads[i].joinable())
					{
						this->threads[i].join();
					}
				}

				close(primary);
				this->watchlist[0].fd = -1;
			}


			void Add(int client)
			{
				std::lock_guard lock(this->cs);

				if (client != -1)
				{
					// First watchlist item is the primary
					for (int i=1; i<CONNECTIONS; i++)
					{
						auto *watch = &this->watchlist[i];

						if (watch->fd < 0)
						{
							if (this->threads[i].joinable())
							{
								this->threads[i].join();
							}

							Log::Info("usock::Server client %d connected", client);

							// Set a timeout for reads on this socket so that recv does
							// not block indefinitely when the server is exiting with
							// outstanding connections.
							timeval tv = { 1, 0 };
							setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(timeval));

							// Do not monitor for POLLIN events since the thread will
							// just block read repeatedly. The main thread will still
							// see the POLLHUP event when the client disconnects.
							*watch = { client, 0, 0 };

							this->threads[i] = std::thread([watch, this] {
								while (watch->fd >= 0)
								{
									this->Read(watch);
								}
							});

							return;
						}
					}

					close(client);

					Log::Error("usock::Server could not handle client %d, no available connections", client);
				}
			}


			bool Close(pollfd *client, int error = 0)
			{
				std::lock_guard lock(this->cs);

				if (client->fd >= 0)
				{
					close(client->fd);

					if (error)	Log::Error("usock::Server client %d disconnected due to error: %s", client->fd, strerror(error));
					else		Log::Info("usock::Server client %d disconnected", client->fd);

					client->fd = -1;
					this->Refresh();
				}

				return false;
			}


			// Refresh the count of used items in the watchlist by finding
			// the last one that has been assigned
			void Refresh()
			{
				this->used = 1;

				for (int i=CONNECTIONS-1; i>0; i--)
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
				static thread_local std::string input(1024, '\0');
				static thread_local std::string output(1024, '\0');

				uint32_t size	= 0;
				uint32_t total	= 0;

				int rc = recv(client->fd, &size, sizeof(uint32_t), 0);


				if (rc != sizeof(uint32_t))
				{
					// The read timed out which is not actually an error since a receive
					// timeout was configured for this connection
					if (errno == EWOULDBLOCK || errno == EAGAIN)
					{
						return false;
					}

					return this->Close(client, rc < 0 ? errno : 0);
				}

				input.resize(size);

				while (total < size)
				{
					rc = recv(client->fd, (byte *)input.data() + total, size - total, 0);

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

				return true;
			}


			void Write(pollfd *client, const std::string &buffer)
			{
				uint32_t size	= buffer.size();
				uint32_t total	= 0;

				int rc = send(client->fd, &size, sizeof(uint32_t), MSG_NOSIGNAL);

				if (rc != sizeof(uint32_t))
				{
					return this->Close(client, rc < 0 ? errno : 0);
				}

				while (total < size)
				{
					rc = send(client->fd, buffer.data() + total, size - total, MSG_NOSIGNAL);

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


			std::function<bool(const std::string&, std::string&)> onMessage;

			std::atomic<bool> run;
			std::array<pollfd, CONNECTIONS> watchlist;
			std::array<std::thread, CONNECTIONS> threads;
			std::mutex cs;
			int used = 0;
	};
}}
