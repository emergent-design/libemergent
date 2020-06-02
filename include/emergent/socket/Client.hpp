#pragma once

#include <emergent/Emergent.hpp>
#include <emergent/logger/Logger.hpp>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


namespace emergent {
namespace usock
{
	// A simple unix socket client interface
	class Client
	{
		public:

			bool Initialise(const std::string path)
			{
				this->Close(0);
				this->path = path;

				return this->Connect();
			}


			bool Write(const std::string &buffer)
			{
				if (!this->Connect()) return false;

				uint32_t total	= 0;
				uint32_t size	= buffer.size();

				// Write header so that the server knows how much data to expect
				int rc = send(this->fd, &size, sizeof(uint32_t), MSG_NOSIGNAL);

				if (rc != sizeof(uint32_t))
				{
					return this->Close(errno);
				}

				while (total < size)
				{
					rc = send(this->fd, buffer.data() + total, size - total, MSG_NOSIGNAL);

					if (rc < 1)
					{
						return this->Close(errno);
					}

					total += rc;
				}

				return true;
			}


			bool Read(std::string &buffer)
			{
				if (!this->Connect()) return false;

				uint32_t size	= 0;
				uint32_t total	= 0;
				int rc			= recv(this->fd, &size, sizeof(uint32_t), 0);

				if (rc != sizeof(uint32_t))
				{
					return this->Close(errno);
				}

				if (size == 0)
				{
					buffer.clear();
					return true;
				}

				buffer.resize(size);

				while (total < size)
				{
					rc = recv(this->fd, (byte *)buffer.data() + total, size - total, 0);

					if (rc < 1)
					{
						return this->Close(errno);
					}

					total += rc;
				}

				return true;
			}


			~Client()
			{
				if (this->fd != -1)
				{
					close(this->fd);
				}
			}


		private:

			bool Connect()
			{
				if (this->fd >= 0) return true;

				this->fd = socket(AF_UNIX, SOCK_STREAM, 0);

				if (this->fd != -1)
				{
					sockaddr_un address	= { 0 };
					address.sun_family	= AF_UNIX;

					strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);

					if (connect(this->fd, (sockaddr *)&address, sizeof(address)) == 0)
					{
						Log::Info("usock::Client connected to %s", this->path);
						return true;
					}

					close(this->fd);
					this->fd = -1;
				}

				return this->Close(errno);
			}


			bool Close(int error)
			{
				if (this->fd != -1)
				{
					close(this->fd);
					this->fd = -1;
				}

				if (error)
				{
					Log::Info("usock::Client error: %s", strerror(error));
				}

				return false;
			}


			int fd = -1;
			std::string path;
	};

}}
