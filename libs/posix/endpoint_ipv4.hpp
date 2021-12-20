#ifndef SOCA_PORT_POSIX_ENDPOINT_IPV4_HPP__
#define SOCA_PORT_POSIX_ENDPOINT_IPV4_HPP__

#include <cstring>
#include <cstdint>
#include "../error.hpp"

#include "port.hpp"

namespace Soca{
namespace POSIX{

class endpoint_ipv4{
	public:
		using native_type = sockaddr_in;
		static constexpr const sa_family_t ep_family = AF_INET;

		constexpr sa_family_t family() const noexcept
		{
			return ep_family;
		}

		endpoint_ipv4()
		{
			std::memset(&addr_, 0, sizeof(native_type));
		}

		endpoint_ipv4(uint16_t port)
		{
			set(port);
		}

		endpoint_ipv4(in_addr_t addr, std::uint16_t port)
		{
			set(addr, port);
		}

		endpoint_ipv4(const char* addr_str, std::uint16_t port, Error& ec)
		{
			if(!set(addr_str, port))
				ec = errc::endpoint_error;
		}

		endpoint_ipv4(const endpoint_ipv4&) = default;

		void set(in_addr_t addr, std::uint16_t port) noexcept
		{
			addr_.sin_family = endpoint_ipv4::ep_family;
			addr_.sin_port = htons(port);
			addr_.sin_addr.s_addr = addr;
		}

		bool set(const char* addr_str, std::uint16_t port) noexcept
		{
			in_addr_t addr;
			int ret = inet_pton(endpoint_ipv4::ep_family, addr_str, &addr);
			if(ret <= 0)
			{
				std::memset(&addr_, 0, sizeof(native_type));
				return false;
			}

			addr_.sin_family = endpoint_ipv4::ep_family;
			addr_.sin_port = htons(port);
			addr_.sin_addr.s_addr = addr;

			return true;
		}

		void set(uint16_t port) noexcept
		{
			addr_.sin_family = AF_INET;
			addr_.sin_addr.s_addr = 0;
			addr_.sin_port = htons(port);
		}

		native_type* native() noexcept{ return &addr_; }

		const char* address(char* addr_str, std::size_t len = INET_ADDRSTRLEN) const noexcept
		{
			return inet_ntop(ep_family, &addr_.sin_addr, addr_str, len);
		}

		const char* host(char* host_addr, std::size_t len = INET_ADDRSTRLEN) const noexcept
		{
			return address(host_addr, len);
		}

		in_addr_t address() noexcept{ return addr_.sin_addr.s_addr; }
		std::uint16_t port() const noexcept{ return ntohs(addr_.sin_port); }

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

		endpoint_ipv4& operator=(endpoint_ipv4 const& ep) noexcept
		{
			addr_.sin_family = ep.addr_.sin_family;
			addr_.sin_port = ep.addr_.sin_port;
			addr_.sin_addr.s_addr = ep.addr_.sin_addr.s_addr;
			return *this;
		}

		bool operator==(endpoint_ipv4 const& ep) const noexcept
		{
			return addr_.sin_port == ep.addr_.sin_port &&
					addr_.sin_addr.s_addr == ep.addr_.sin_addr.s_addr;
		}

		bool operator!=(endpoint_ipv4 const& ep) const noexcept
		{
			return !(*this == ep);
		}
	private:
		native_type		addr_;
};

}//POSIX
}//Soca

#endif /* SOCA_PORT_POSIX_ENDPOINT_IPV4_HPP__ */
