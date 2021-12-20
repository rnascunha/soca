#ifndef SOCA_POSIX_ENDPOINT_IPV6_HPP__
#define SOCA_POSIX_ENDPOINT_IPV6_HPP__

#include <cstring>
#include <cstdint>
#include "../error.hpp"

#include "port.hpp"

namespace Soca{
namespace POSIX{

inline bool operator==(in6_addr const& lhs, in6_addr const& rhs) noexcept
{
	return std::memcmp(&lhs, &rhs, sizeof(in6_addr)) == 0;
}

class endpoint_ipv6{
	public:
		using native_type = sockaddr_in6;
		static constexpr const sa_family_t ep_family = AF_INET6;

		constexpr sa_family_t family() const noexcept
		{
			return ep_family;
		}

		endpoint_ipv6()
		{
			std::memset(&addr_, 0, sizeof(native_type));
		}

		endpoint_ipv6(std::uint16_t port)
		{
			set(port);
		}

		endpoint_ipv6(in6_addr addr, std::uint16_t port)
		{
			set(addr, port);
		}

		endpoint_ipv6(const char* addr_str, std::uint16_t port, Error& ec)
		{
			if(!set(addr_str, port))
				ec = errc::endpoint_error;
		}

		endpoint_ipv6(const endpoint_ipv6&) = default;

		void set(in6_addr const& addr, std::uint16_t port) noexcept
		{
			std::memset(&addr_, 0, sizeof(native_type));
			addr_.sin6_family = ep_family;
			addr_.sin6_port = htons(port);
			addr_.sin6_addr = addr;
			std::memcpy(&addr_.sin6_addr, &addr, sizeof(in6_addr));
		}

		bool set(const char* addr_str, std::uint16_t port) noexcept
		{
			in6_addr addr;
			std::memset(&addr_, 0, sizeof(native_type));
			
			int ret = inet_pton(ep_family, addr_str, &addr);
			if(ret <= 0)
			{
				return false;
			}

			addr_.sin6_family = ep_family;
			addr_.sin6_port = htons(port);
			std::memcpy(&addr_.sin6_addr, &addr, sizeof(in6_addr));

			return true;
		}

		void set(uint16_t port) noexcept
		{
			addr_.sin6_family = AF_INET6;
			std::memset(&addr_.sin6_addr, 0, sizeof(addr_.sin6_addr));
			addr_.sin6_port = htons(port);
		}

		native_type* native() noexcept{ return &addr_; }

		const char* address(char* addr_str, std::size_t len = INET6_ADDRSTRLEN) const noexcept
		{
			return inet_ntop(ep_family, &addr_.sin6_addr, addr_str, len);
		}
		
		const char* host(char* host_addr, std::size_t len = INET6_ADDRSTRLEN) const noexcept
		{
			return address(host_addr, len);
		}

		in6_addr address() noexcept{ return addr_.sin6_addr; }
		std::uint16_t port() const noexcept{ return ntohs(addr_.sin6_port); }

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

		endpoint_ipv6& operator=(endpoint_ipv6 const& ep) noexcept
		{
			addr_.sin6_family = ep.addr_.sin6_family;
			addr_.sin6_port = ep.addr_.sin6_port;
			std::memcpy(&addr_.sin6_addr, &ep.addr_.sin6_addr, sizeof(in6_addr));
			return *this;
		}

		bool operator==(endpoint_ipv6 const& ep) const noexcept
		{
			return addr_.sin6_port == ep.addr_.sin6_port &&
					addr_.sin6_addr == ep.addr_.sin6_addr;
		}

		bool operator!=(endpoint_ipv6 const& ep) const noexcept
		{
			return !(*this == ep);
		}
	private:
		native_type		addr_;
};

}//POSIX
}//Soca

#endif /* SOCA_POSIX_ENDPOINT_IPV6_HPP__ */
