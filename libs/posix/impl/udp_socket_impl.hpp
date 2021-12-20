#ifndef SOCA_POSIX_SOCKET_UDP_IMPL_HPP__
#define SOCA_POSIX_SOCKET_UDP_IMPL_HPP__

#include "../udp_socket.hpp"
#include "../functions.hpp"

#include <cerrno>

namespace Soca{
namespace POSIX{

template<class Endpoint,
		int Flags>
udp<Endpoint, Flags>::
udp() : socket_(0){}

template<class Endpoint,
		int Flags>
void
udp<Endpoint, Flags>::
open(Error& ec) noexcept
{
	if((socket_ = ::socket(endpoint::ep_family, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		ec = errc::socket_error;
		return;
	}
	if constexpr((Flags & MSG_DONTWAIT) != 0)
		nonblock_socket(socket_);
}

template<class Endpoint,
		int Flags>
void
udp<Endpoint, Flags>::
open(sa_family_t family, Error& ec) noexcept
{
	if((socket_ = ::socket(family, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		ec = errc::socket_error;
		return;
	}
	if constexpr((Flags & MSG_DONTWAIT) != 0)
		nonblock_socket(socket_);

}

template<class Endpoint,
		int Flags>
void
udp<Endpoint, Flags>::
open(endpoint& ep, Error& ec) noexcept
{
	if((socket_ = ::socket(ep.family(), SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		ec = errc::socket_error;
		return;
	}
	if constexpr((Flags & MSG_DONTWAIT) != 0)
		nonblock_socket(socket_);

	bind(ep, ec);
}

template<class Endpoint,
		int Flags>
void
udp<Endpoint, Flags>::
bind(endpoint& ep, Error& ec) noexcept
{
	if (::bind(socket_,
		reinterpret_cast<struct sockaddr const*>(ep.native()),
		sizeof(typename endpoint::native_type)) == -1)
	{
		ec = errc::socket_bind;
	}
}

template<class Endpoint,
		int Flags>
void
udp<Endpoint, Flags>::
close() noexcept
{
	::shutdown(socket_, SHUT_RDWR);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	::closesocket(socket_);
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	::close(socket_);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	socket_ = 0;
}

template<class Endpoint,
		int Flags>
std::size_t
udp<Endpoint, Flags>::
send(const void* buffer, std::size_t buffer_len, endpoint& ep, Error& ec) noexcept
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	int sent = ::sendto(socket_, static_cast<const char*>(buffer), static_cast<int>(buffer_len), 0,
				reinterpret_cast<struct sockaddr const*>(ep.native()),
				sizeof(typename endpoint::native_type));
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	int sent = ::sendto(socket_, buffer, buffer_len, 0,
				reinterpret_cast<struct sockaddr const*>(ep.native()),
				sizeof(typename endpoint::native_type));
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	if(sent < 0)
	{
		ec = errc::socket_send;
		return 0;
	}

	return sent;
}

template<class Endpoint,
		int Flags>
std::size_t
udp<Endpoint, Flags>::
receive(void* buffer, std::size_t buffer_len, endpoint& ep, Error& ec) noexcept
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	/**
	 * sockaddr_storage fits both IPv4 and IPv6
	 */
	int addr_len = sizeof(struct sockaddr_storage);
	int recv = ::recvfrom(socket_,
			static_cast<char*>(buffer), static_cast<int>(buffer_len), 0,
			reinterpret_cast<struct sockaddr*>(ep.native()), &addr_len);
#else /* #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	/**
	 * sockaddr_storage fits both IPv4 and IPv6
	 */
	unsigned addr_len = sizeof(struct sockaddr_storage);
	int recv = ::recvfrom(socket_,
			buffer, buffer_len, 0,
			reinterpret_cast<struct sockaddr*>(ep.native()), &addr_len);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */

	if(recv < 0)
	{
		if constexpr((Flags & MSG_DONTWAIT) != 0)
		{
#if	defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			if(WSAGetLastError() == WSAEWOULDBLOCK)
#else
			if(errno == EAGAIN || errno == EWOULDBLOCK)
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
				return 0;
		}
		ec = errc::socket_receive;
		return 0;
	}

	return recv;
}

template<class Endpoint,
		int Flags>
template<int BlockTimeMs>
std::size_t
udp<Endpoint, Flags>::
receive(void* buffer, std::size_t buffer_len, endpoint& ep, Error& ec) noexcept
{
	struct timeval tv = {
		/*.tv_sec = */BlockTimeMs / 1000,
		/*.tv_usec = */(BlockTimeMs % 1000) * 1000
	};

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(socket_, &rfds);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	//Using socket_ + 1, gives warning... (but why)... this value is not used at Windows
	int s = select(0, &rfds, NULL, NULL, BlockTimeMs  < 0 ? NULL : &tv);
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	int s = select(socket_ + 1, &rfds, NULL, NULL, BlockTimeMs  < 0 ? NULL : &tv);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	if(s < 0)
	{
		ec = errc::socket_receive;
		return 0;
	}
	if(FD_ISSET(socket_, &rfds))
	{
		return receive(buffer, buffer_len, ep, ec);
	}
	return 0;
}

}//POSIX
}//Soca

#endif /* SOCA_POSIX_SOCKET_UDP_IMPL_HPP__ */
