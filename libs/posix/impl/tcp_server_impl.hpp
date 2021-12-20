#ifndef SOCA_POSIX_SOCKET_TCP_SERVER_IMPL_HPP__
#define SOCA_POSIX_SOCKET_TCP_SERVER_IMPL_HPP__

#include "../tcp_server.hpp"
#include "../port.hpp"

#include <type_traits>

namespace Soca{
namespace POSIX{

template<class Endpoint,
		int Flags>
tcp_server<Endpoint, Flags>::tcp_server()
	: socket_(0)
#if SOCA_USE_SELECT != 1
	  , epoll_fd_(0)
#endif /* SOCA_USE_SELECT != 1 */
{
#if SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1
	FD_ZERO(&list_);
#endif /* SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1 */
}

template<class Endpoint,
		int Flags>
template<int PendingQueueSize /* = 10 */>
void
tcp_server<Endpoint, Flags>::
open(endpoint& ep, Error& ec) noexcept
{
	if((socket_ = ::socket(ep.family(), SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		ec = errc::socket_error;
		return;
	}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	char opt = 1;
#else
	int opt = 1;
#endif
	if(::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	{
		close();
		ec = errc::socket_error;
		return;
	}

	if (::bind(socket_,
		reinterpret_cast<struct sockaddr const*>(ep.native()),
		sizeof(typename endpoint::native_type)) == -1)
	{
		close();
		ec = errc::socket_bind;
		return;
	}

	if(::listen(socket_, PendingQueueSize) == -1)
	{
		close();
		ec = errc::socket_error;
		return;
	}

	if constexpr((Flags & MSG_DONTWAIT) != 0)
		nonblock_socket(socket_);

	if(!open_poll())
	{
		close();
		ec = errc::socket_error;
	}
}

template<class Endpoint,
		int Flags>
bool tcp_server<Endpoint, Flags>::
is_open() const noexcept
{
	return socket_ != 0;
}

template<class Endpoint,
		int Flags>
bool
tcp_server<Endpoint, Flags>::
open_poll() noexcept
{
#if SOCA_USE_SELECT != 1
	epoll_fd_ = epoll_create1(0);
	if(epoll_fd_ == -1)
		return false;

	if(!add_socket_poll(socket_, EPOLLIN | EPOLLOUT | EPOLLET))
		return false;
#endif /* SOCA_USE_SELECT = 1 */
	return true;
}

template<class Endpoint,
		int Flags>
bool
tcp_server<Endpoint, Flags>::
add_socket_poll(handler socket, std::uint32_t events [[maybe_unused]]) noexcept
{
#if SOCA_USE_SELECT != 1
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = socket;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket, &ev) == -1)
		return false;
#endif
#if SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1
	FD_SET(socket, &list_);
#endif /* SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1 */
	return true;
}

template<class Endpoint,
		int Flags>
void tcp_server<Endpoint, Flags>::
close() noexcept
{
#if SOCA_USE_SELECT != 1
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, socket_, NULL);
	if(epoll_fd_) ::close(epoll_fd_);
	epoll_fd_ = 0;
#endif /* SOCA_USE_SELECT == 1 */
#if SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1
	FD_ZERO(&list_);
#endif /* SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1 */
	if(socket_)
	{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		::closesocket(socket_);
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
		::close(socket_);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	}
	socket_ = 0;
}

template<class Endpoint,
	int Flags>
void tcp_server<Endpoint, Flags>::
close_client(handler socket) noexcept
{
#if SOCA_USE_SELECT != 1
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, socket, NULL);
#endif /* SOCA_USE_SELECT != 1 */
#if SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1
	FD_CLR(socket, &list_);
#endif /* SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1 */
	::shutdown(socket, SHUT_RDWR);
	
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	::closesocket(socket);
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	::close(socket);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
}

template<class Endpoint,
		int Flags>
typename tcp_server<Endpoint, Flags>::handler
tcp_server<Endpoint, Flags>::
accept(Error& ec) noexcept
{
	handler s = 0;
	endpoint ep;
	socklen_t len = sizeof(typename endpoint::native_type);
	if((s = ::accept(socket_, reinterpret_cast<struct sockaddr*>(ep.native()), &len)) == -1)
	{
		ec = errc::socket_error;
	}
	else
	{
#if SOCA_USE_SELECT != 1
		if(!add_socket_poll(s, EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP))
#else /* SOCA_USE_SELECT != 1 */
		if(!add_socket_poll(s, 0))
#endif /* SOCA_USE_SELECT != 1 */
			ec = errc::socket_error;
		if constexpr((Flags & MSG_DONTWAIT) != 0)
			nonblock_socket(s);
	}
	return s;
}

#if SOCA_USE_SELECT != 1

template<class Endpoint,
		int Flags>
template<
		int BlockTimeMs /* = 0 */,
		unsigned MaxEvents /* = 32 */,
		typename ReadCb,
		typename OpenCb /* = void* */,
		typename CloseCb /* = void* */>
bool
tcp_server<Endpoint, Flags>::
run(Error& ec,
		ReadCb read_cb,
		OpenCb open_cb/* = nullptr */ [[maybe_unused]],
		CloseCb close_cb/* = nullptr */ [[maybe_unused]]) noexcept
{
	struct epoll_event events[MaxEvents];

	int event_num = epoll_wait(epoll_fd_, events, MaxEvents, BlockTimeMs);
	for (int i = 0; i < event_num; i++)
	{
		if (events[i].data.fd == socket_)
		{
			[[maybe_unused]] int c = accept(ec);
			if constexpr(!std::is_same<void*, OpenCb>::value)
			{
				open_cb(c);
			}
		}
		else if (events[i].events & EPOLLIN)
		{
			/* handle EPOLLIN event */
			handler s = events[i].data.fd;
			read_cb(s);
		}
		/* check if the connection is closing */
		if (events[i].events & (EPOLLRDHUP | EPOLLHUP))
		{
			handler s = events[i].data.fd;
			if constexpr(!std::is_same<void*, CloseCb>::value)
			{
				close_cb(s);
			}
			close_client(s);
		}
	}
	return ec ? false : true;
}

#else /* SOCA_USE_SELECT == 1 */

template<class Endpoint,
		int Flags>
template<
		int BlockTimeMs /* = 0 */,
		unsigned MaxEvents /* = 32 */,
		typename ReadCb,
		typename OpenCb /* = void* */,
		typename CloseCb /* = void* */>
bool
tcp_server<Endpoint, Flags>::
run(Error& ec,
		ReadCb read_cb,
		OpenCb open_cb/* = nullptr */ [[maybe_unused]],
		CloseCb close_cb/* = nullptr */ [[maybe_unused]]) noexcept
{
	fd_set rfds;

	struct timeval tv = {
		/*.tv_sec = */BlockTimeMs / 1000,
		/*.tv_usec = */(BlockTimeMs % 1000) * 1000
	};

	std::memcpy(&rfds, &list_, sizeof(fd_set));
	FD_SET(socket_, &rfds);

	int max = 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	for(unsigned i = 0; i < FD_SETSIZE; i++)
	{
		if(FD_ISSET(i, &rfds)) max = i;
	}
	max = max > socket_ ? max : socket_;
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */

	int s = select(max + 1, &rfds, NULL, NULL, BlockTimeMs  < 0 ? NULL : &tv);
	if(s < 0)
	{
		ec = errc::socket_error;
		return false;
	}
	
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	for(unsigned i = 0, count = 0; count < rfds.fd_count && i < FD_SETSIZE; i++)
	{
		if(FD_ISSET(rfds.fd_array[i], &rfds))
		{
			if(rfds.fd_array[i] == socket_)
			{
				[[maybe_unused]] handler c = accept(ec);
				if constexpr(!std::is_same<void*, OpenCb>::value)
				{
					open_cb(c);
				}
			}
			else if(!read_cb(rfds.fd_array[i]))
			{
				if constexpr(!std::is_same<void*, CloseCb>::value)
				{
					close_cb(rfds.fd_array[i]);
				}
				close_client(rfds.fd_array[i]);
			}
			count++;
		}
	}
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	for(int i = 1, count = 0; i <= max && count < s; i++)
	{
		if(FD_ISSET(i, &rfds))
		{
			if(i == socket_)
			{
				[[maybe_unused]] handler c = accept(ec);
				if constexpr(!std::is_same<void*, OpenCb>::value)
				{
					open_cb(c);
				}
			}
			else if(!read_cb(i))
			{
				if constexpr(!std::is_same<void*, CloseCb>::value)
				{
					close_cb(i);
				}
				close_client(i);
			}
			count++;
		}
	}
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	return ec ? false : true;
}

#endif /* SOCA_USE_SELECT == 1 */

template<class Endpoint,
		int Flags>
std::size_t
tcp_server<Endpoint, Flags>::
receive(handler socket, void* buffer, std::size_t buffer_len, Error& ec) noexcept
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	ssize_t bytes = ::recv(socket, static_cast<char*>(buffer), static_cast<int>(buffer_len), 0);
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	ssize_t bytes = ::recv(socket, buffer, buffer_len, 0);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	if(bytes < 1)
	{
		if constexpr((Flags & MSG_DONTWAIT) != 0)
		{
#if	defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			if(bytes == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
			if(bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
			{
				return 0;
			}
		}
		ec = errc::socket_receive;
		return 0;
	}
	return bytes;
}

template<class Endpoint,
		int Flags>
std::size_t
tcp_server<Endpoint, Flags>::
send(handler to_socket, const void* buffer, std::size_t buffer_len, Error& ec)  noexcept
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	int size = ::send(to_socket, static_cast<const char*>(buffer), static_cast<int>(buffer_len), 0);
#else /* #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	int size = ::send(to_socket, buffer, buffer_len, 0);
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
	if(size < 0)
	{
		if constexpr((Flags & MSG_DONTWAIT) != 0)
		{
#if	defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
			if(WSAGetLastError() == WSAEWOULDBLOCK)
#else
			if(errno == EAGAIN || errno == EWOULDBLOCK)
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
			{
				return 0;
			}
		}
		ec = errc::socket_send;
		return 0;
	}
	return size;
}

#if SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1
template<class Endpoint,
		int Flags>
fd_set const&
tcp_server<Endpoint, Flags>::
client_list() const noexcept
{
	return list_;
}
#endif /* SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1 */

}//POSIX
}//Soca

#endif /* SOCA_POSIX_SOCKET_TCP_SERVER_IMPL_HPP__ */
