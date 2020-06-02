#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/logger/Logger.hpp>
#include <emergent/thread/Persistent.hpp>

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
	using namespace std::chrono_literals;

	// A multi-threaded unix socket implementation. Each connection is assumed to be
	// long-lived and so a thread is allocated to manage the read/write cycle of a
	// single connection.
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
							this->threads[0].Run(std::bind(&Server::Entry, this));

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

				Log::Info("usock::Server waiting for threads to finish");

				while (std::any_of(
					this->threads.begin(),
					this->threads.end(),
					[](auto &t) { return !t.Ready(); }
				))
				{
					std::this_thread::sleep_for(10ms);
				}
			}


		private:

			void Entry()
			{
				const int primary = this->watchlist[0].fd;

				while (this->run)
				{
					int rc = poll(this->watchlist.data(), this->used, 10);

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
								close(w.fd);
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

				// Close any remaining connections
				for (int i=1; i<CONNECTIONS; i++)
				{
					if (this->watchlist[i].fd >= 0)
					{
						close(this->watchlist[i].fd);
					}
				}

				close(primary);

				Log::Info("usock::Server exited");
			}


			void Add(int client)
			{
				if (client != -1)
				{
					// First watchlist item is the primary
					for (int i=1; i<CONNECTIONS; i++)
					{
						auto *watch = &this->watchlist[i];

						if (watch->fd < 0 && this->threads[i].Ready())
						{
							// Set a timeout for reads on this socket so that recv does
							// not block indefinitely when the server is exiting with
							// outstanding connections.
							timeval tv = { 1, 0 };
							setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(timeval));

							// Do not monitor for POLLIN events since the thread will
							// just block read repeatedly. The main thread will still
							// see the POLLHUP event when the client disconnects.
							*watch = { client, 0, 0 };

							this->threads[i].Run([watch, i, this] {

								Log::Info("usock::Server client %d connected on thread %d", watch->fd, i);

								while (this->Read(watch->fd));

								Log::Info("usock::Server client %d disconnected on thread %d", watch->fd, i);

								close(watch->fd);
								watch->fd = -1;
							});

							this->Refresh();
							return;
						}
					}

					close(client);

					Log::Error("usock::Server could not handle client %d, no available connections", client);
				}
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


			bool Read(int client)
			{
				thread_local std::string input(1024, '\0');
				thread_local std::string output(1024, '\0');

				uint32_t size	= 0;
				uint32_t total	= 0;

				int rc = recv(client, &size, sizeof(uint32_t), 0);


				if (rc != sizeof(uint32_t))
				{
					// The read timed out which is not actually an error since a receive
					// timeout was configured for this connection
					if (errno == EWOULDBLOCK || errno == EAGAIN)
					{
						return true;
					}

					if (rc < 0)
					{
						Log::Error("usock::Server client %d error: %s", client, strerror(errno));
					}

					return false;
				}

				input.resize(size);

				while (total < size)
				{
					rc = recv(client, (byte *)input.data() + total, size - total, 0);

					if (rc < 1)
					{
						Log::Error("usock::Server client %d error: %s", client, strerror(errno));
						return false;
					}
					else
					{
						total += rc;
					}
				}

				if (total && this->onMessage && this->onMessage(input, output))
				{
					return this->Write(client, output);
				}

				return true;
			}


			bool Write(int client, const std::string &buffer)
			{
				uint32_t size	= buffer.size();
				uint32_t total	= 0;

				int rc = send(client, &size, sizeof(uint32_t), MSG_NOSIGNAL);

				if (rc != sizeof(uint32_t))
				{
					if (rc < 0)
					{
						Log::Error("usock::Server client %d error: %s", client, strerror(errno));
					}

					return false;
				}

				while (total < size)
				{
					rc = send(client, buffer.data() + total, size - total, MSG_NOSIGNAL);

					if (rc < 1)
					{
						Log::Error("usock::Server client %d error: %s", client, strerror(errno));
						return false;
					}
					else
					{
						total += rc;
					}
				}

				return true;
			}


			std::function<bool(const std::string&, std::string&)> onMessage;
			std::array<PersistentThread, CONNECTIONS> threads;
			std::array<pollfd, CONNECTIONS> watchlist;
			std::atomic<bool> run;
			int used = 0;
	};
}}
