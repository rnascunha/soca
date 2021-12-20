#ifndef SOCA_POSIX_ENDPOINT_IP_HPP__
#define SOCA_POSIX_ENDPOINT_IP_HPP__

#include <cstring>
#include <cstdint>
#include "../error.hpp"

#include "port.hpp"

namespace Soca{
namespace POSIX{

class endpoint_ip{
	public:
		using native_type = sockaddr_storage;

		sa_family_t family() const noexcept
		{
			return family_;
		}

		endpoint_ip()
		{
			std::memset(&addr_, 0, sizeof(native_type));
		}

		endpoint_ip(sa_family_t family, uint16_t port)
		{
			set(family, port);
		}

		endpoint_ip(in_addr_t addr, std::uint16_t port)
		{
			set(addr, port);
		}

		endpoint_ip(in6_addr addr, std::uint16_t port)
		{
			set(addr, port);
		}

		endpoint_ip(const char* addr_str, std::uint16_t port, Error& ec)
		{
			if(!set(addr_str, port))
				ec = errc::endpoint_error;
		}

		endpoint_ip(const endpoint_ip&) = default;

		void set(in_addr_t addr, std::uint16_t port) noexcept
		{
			struct sockaddr_in addr_in;

			family_ = AF_INET;
			addr_in.sin_family = family_;
			addr_in.sin_port = htons(port);
			addr_in.sin_addr.s_addr = addr;

			std::memcpy(&addr_, &addr_in, sizeof(addr_in));
		}

		void set(in6_addr const& addr, std::uint16_t port) noexcept
		{
			struct sockaddr_in6 addr_in;

			std::memset(&addr_in, 0, sizeof(struct sockaddr_in6));
			family_ = AF_INET6;
			addr_in.sin6_family = AF_INET6;
			addr_in.sin6_port = htons(port);
			addr_in.sin6_addr = addr;
			std::memcpy(&addr_in.sin6_addr, &addr, sizeof(in6_addr));

			std::memcpy(&addr_, &addr_in, sizeof(addr_in));
		}

		bool set(const char* addr_str, std::uint16_t port) noexcept
		{
			//Check IPv4
			in_addr_t addr;
			int ret = inet_pton(AF_INET, addr_str, &addr);
			if(ret > 0)
			{
				set(addr, port);
				return true;
			}

			//Check IPv6
			in6_addr addr6;
			ret = inet_pton(AF_INET6, addr_str, &addr);
			if(ret > 0)
			{
				set(addr6, port);
				return true;
			}

			std::memset(&addr_, 0, sizeof(native_type));
			return false;
		}

		void set(sa_family_t family, uint16_t port) noexcept
		{
			family_ = family;
			if(family == AF_INET)
			{
				struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(&addr_);
				addr->sin_family = AF_INET;
				addr->sin_addr.s_addr = 0;
				addr->sin_port = htons(port);
			}
			else
			{
				struct sockaddr_in6* addr = reinterpret_cast<struct sockaddr_in6*>(&addr_);
				addr->sin6_family = AF_INET6;
				std::memset(&addr->sin6_addr, 0, sizeof(addr->sin6_addr));
				addr->sin6_port = htons(port);
			}
		}

		native_type* native() noexcept{ return &addr_; }

		const char* address(char* addr_str, std::size_t len = INET6_ADDRSTRLEN) const noexcept
		{
			if(family_ == AF_INET)
			{
				struct sockaddr_in const* addr = reinterpret_cast<struct sockaddr_in const*>(&addr_);
				return inet_ntop(family_, &addr->sin_addr, addr_str, len);
			}
			else
			{
				struct sockaddr_in6 const* addr = reinterpret_cast<struct sockaddr_in6 const*>(&addr_);
				return inet_ntop(family_, &addr->sin6_addr, addr_str, len);
			}
		}

		const char* host(char* host_addr, std::size_t len = INET6_ADDRSTRLEN) const noexcept
		{
			return address(host_addr, len);
		}

		in_addr_t address() const noexcept
		{
			struct sockaddr_in const* addr = reinterpret_cast<struct sockaddr_in const*>(&addr_);
			return addr->sin_addr.s_addr;
		}

		in6_addr const& address6() const noexcept
		{
			struct sockaddr_in6 const* addr = reinterpret_cast<struct sockaddr_in6 const*>(&addr_);
			return addr->sin6_addr;
		}

		std::uint16_t port() const noexcept
		{
			if(family_ == AF_INET)
			{
				struct sockaddr_in const* addr = reinterpret_cast<struct sockaddr_in const*>(&addr_);
				return ntohs(addr->sin_port);
			}
			else
			{
				struct sockaddr_in6 const* addr = reinterpret_cast<struct sockaddr_in6 const*>(&addr_);
				return ntohs(addr->sin6_port);
			}
		}

		template<typename Handler>
		bool copy_sock_address(Handler socket) noexcept
		{
			socklen_t size = sizeof(native_type);
			if(::getsockname(socket,
					reinterpret_cast<sockaddr*>(&addr_),
					&size) == -1)
				return false;
			return true;
		}

		template<typename Handler>
		bool copy_peer_address(Handler socket) noexcept
		{
			socklen_t size = sizeof(native_type);
			if(::getpeername(socket,
					reinterpret_cast<sockaddr*>(&addr_),
					&size) == -1)
				return false;
			return true;
		}

		endpoint_ip& operator=(endpoint_ip const& ep) noexcept
		{
			family_ = ep.family();
			if(ep.family() == AF_INET)
			{
				struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(&addr_);
				addr->sin_family = ep.family();
				addr->sin_port = ep.port();
				addr->sin_addr.s_addr = ep.address();
			}
			else
			{
				struct sockaddr_in6* addr = reinterpret_cast<struct sockaddr_in6*>(&addr_);
				addr->sin6_family = ep.family();
				addr->sin6_port = ep.port();
				std::memcpy(&addr->sin6_addr, &ep.address6(), sizeof(in6_addr));
			}
			return *this;
		}

		bool operator==(endpoint_ip const& ep) const noexcept
		{
			if(family_ != ep.family()) return false;
			if(family_ == AF_INET)
			{
				struct sockaddr_in const* addr = reinterpret_cast<struct sockaddr_in const*>(&addr_);
				return addr->sin_port == ep.port() &&
						addr->sin_addr.s_addr == ep.address();
			}
			else
			{
				struct sockaddr_in6 const* addr = reinterpret_cast<struct sockaddr_in6 const*>(&addr_);
				return addr->sin6_port == ep.port() &&
						std::memcmp(&addr->sin6_addr, &ep.address6(), sizeof(in6_addr)) == 0;
			}
		}

		bool operator!=(endpoint_ip const& ep) const noexcept
		{
			return !(*this == ep);
		}
	private:
		native_type		addr_;
		sa_family_t		family_ = AF_INET;
};

}//POSIX
}//Soca

#endif /* SOCA_POSIX_ENDPOINT_IP_HPP__ */
