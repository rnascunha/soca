#ifndef SOCA_POSIX_SOCKET_TCP_SERVER_HPP__
#define SOCA_POSIX_SOCKET_TCP_SERVER_HPP__

#include <cstdlib>
#include <cstdint>

#include "../error.hpp"
#include "port.hpp"

namespace Soca{
namespace POSIX{

template<class Endpoint,
		int Flags = MSG_DONTWAIT>
class tcp_server{
	public:
		static constexpr bool set_length = true;
		static constexpr bool is_server = true;
#if	defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		using handler = SOCKET;
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
		using handler = int;
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
		using endpoint = Endpoint;

		tcp_server();

		template<int PendingQueueSize = 10>
		void open(endpoint&, Error&) noexcept;
		bool is_open() const noexcept;

		template<
			int BlockTimeMs = 0,
			unsigned MaxEvents = 32,
			typename ReadCb,
			typename OpenCb = void*,
			typename CloseCb = void*>
		bool run(Error&,
				ReadCb, OpenCb = nullptr, CloseCb = nullptr) noexcept;

		std::size_t send(handler to_socket, const void*, std::size_t, Error&)  noexcept;
		std::size_t receive(handler socket, void* buffer, std::size_t, Error&) noexcept;

		void close() noexcept;
		void close_client(handler) noexcept;

#if SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1
		fd_set const& client_list() const noexcept;
#endif /* SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1 */
	private:
		handler accept(Error&) noexcept;
		bool open_poll() noexcept;
		bool add_socket_poll(handler socket, std::uint32_t events) noexcept;

		handler socket_;
#if SOCA_USE_SELECT != 1
		int epoll_fd_;
#endif /* SOCA_USE_SELECT != 1 */
#if SOCA_USE_SELECT == 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1
		fd_set	list_;
#endif /* SOCA_USE_SELECT != 1 || SOCA_TCP_SERVER_CLIENT_LIST == 1*/
};

}//POSIX
}//Soca

#include "impl/tcp_server_impl.hpp"

#endif /* SOCA_POSIX_SOCKET_TCP_SERVER_HPP__ */
