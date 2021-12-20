#ifndef SOCA_POSIX_SOCKET_TCP_CLIENT_IMPL_HPP__
#define SOCA_POSIX_SOCKET_TCP_CLIENT_IMPL_HPP__

#include "../tcp_client.hpp"
#include "../functions.hpp"

#include <cerrno>

namespace Soca{
namespace POSIX{

template<class Endpoint,
		int Flags>
tcp_client<Endpoint, Flags>::
tcp_client() : socket_(0){}

template<class Endpoint,
		int Flags>
tcp_client<Endpoint, Flags>::
~tcp_client()
{
	if(is_open()) close();
}

template<class Endpoint,
		int Flags>
void
tcp_client<Endpoint, Flags>::
open(endpoint& ep, Error& ec) noexcept
{
	if((socket_ = ::socket(ep.family(), SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		ec = errc::socket_error;
		return;
	}

	if(::connect(socket_,
		reinterpret_cast<struct sockaddr const*>(ep.native()),
		sizeof(typename endpoint::native_type)) < 0)
	{
		ec = errc::socket_error;
		close();
		return;
	}

	if constexpr((Flags & MSG_DONTWAIT) != 0)
		nonblock_socket(socket_);
}

template<class Endpoint,
		int Flags>
bool
tcp_client<Endpoint, Flags>::
async_open(endpoint& ep, Error& ec) noexcept
{
	if((socket_ = ::socket(ep.family(), SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		ec = errc::socket_error;
		return false;
	}

	if constexpr((Flags & MSG_DONTWAIT) != 0)
		nonblock_socket(socket_);

	int res = ::connect(socket_,
			reinterpret_cast<struct sockaddr const*>(ep.native()),
			sizeof(typename endpoint::native_type));
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	if(res == -1 && WSAGetLastError() == WSAEINPROGRESS)
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	if(res == -1 && errno != EINPROGRESS)
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	{

		ec = errc::socket_error;
		close();
		return false;
	}

	return errno == EINPROGRESS ? false : true;
}

template<class Endpoint,
		int Flags>
template<int BlockTimeMs /* = -1 */>
bool
tcp_client<Endpoint, Flags>::
wait_connect(Error& ec) const noexcept
{
	struct timeval tv = {
		/*.tv_sec = */BlockTimeMs / 1000,
		/*.tv_usec = */(BlockTimeMs % 1000) * 1000
	};

	fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(socket_, &wfds);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	//Using socket_ + 1, gives warning... (but why)... this value is not used at Windows
	int s = select(0, NULL, &wfds, NULL, BlockTimeMs  < 0 ? NULL : &tv);
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	int s = select(socket_ + 1, NULL, &wfds, NULL, BlockTimeMs  < 0 ? NULL : &tv);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	if(s < 0)
	{
		ec = errc::socket_error;
		return false;
	}
	if(FD_ISSET(socket_, &wfds))
	{
		typename endpoint::native_type addr;
		socklen_t size = sizeof(typename endpoint::native_type);
		if(::getpeername(socket_,
			reinterpret_cast<sockaddr*>(&addr),
			&size) == -1)
		{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
			if(errno == ENOTCONN)
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
			{
				ec = errc::socket_error;
				return false;
			}
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			if(WSAGetLastError() == WSAEINPROGRESS)
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */ 
			if(errno == EINPROGRESS)
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
			{
				return false;
			}
		}
		return true;
	}

	return false;
}

template<class Endpoint,
		int Flags>
void
tcp_client<Endpoint, Flags>::
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
bool
tcp_client<Endpoint, Flags>::
is_connected() const noexcept
{
	Error ec;
	return wait_connect<0>(ec);
}

template<class Endpoint,
		int Flags>
bool
tcp_client<Endpoint, Flags>::
is_open() const noexcept
{
	return socket_ != 0;
}

template<class Endpoint,
		int Flags>
void
tcp_client<Endpoint, Flags>::
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
typename tcp_client<Endpoint, Flags>::handler
tcp_client<Endpoint, Flags>::
native() const noexcept
{
	return socket_;
}

template<class Endpoint,
		int Flags>
std::size_t
tcp_client<Endpoint, Flags>::
send(const void* buffer, std::size_t buffer_len, Error& ec) noexcept
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	int sent = ::send(socket_, static_cast<const char*>(buffer), static_cast<int>(buffer_len), 0);
#else /* #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	int sent = ::send(socket_, buffer, buffer_len, 0);
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
tcp_client<Endpoint, Flags>::
receive(void* buffer, std::size_t buffer_len, Error& ec) noexcept
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	int recv = ::recv(socket_, static_cast<char*>(buffer), static_cast<int>(buffer_len), 0);
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	int recv = ::recv(socket_, buffer, buffer_len, 0);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	if(recv < 1)
	{
		if constexpr((Flags & MSG_DONTWAIT) != 0)
		{
#if	defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			if(recv == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
			if(recv == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
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
tcp_client<Endpoint, Flags>::
receive(void* buffer, std::size_t buffer_len, Error& ec) noexcept
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
		return receive(buffer, buffer_len, ec);
	}
	return 0;
}

}//POSIX
}//Soca

#endif /* SOCA_POSIX_SOCKET_TCP_CLIENT_IMPL_HPP__ */
