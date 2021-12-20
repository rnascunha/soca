#ifndef SOCA_POSIX_SOCKET_TCP_CLIENT_HPP__
#define SOCA_POSIX_SOCKET_TCP_CLIENT_HPP__

#include <cstdlib>
#include <cstdint>
#include "../error.hpp"
#include "../port.hpp"

namespace Soca{
namespace POSIX{

template<class Endpoint,
		int Flags = MSG_DONTWAIT>
class tcp_client{
	public:
		static constexpr bool set_length = true;
		static constexpr bool is_server = false;
#if	defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		using handler = SOCKET;
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
		using handler = int;
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
		using endpoint = Endpoint;
		
		tcp_client();
		~tcp_client();

		handler native() const noexcept;

		void open(endpoint&, Error&) noexcept;
		bool async_open(endpoint&, Error&) noexcept;
		template<int BlockTimeMs = -1>
		bool wait_connect(Error&) const noexcept;

		void bind(endpoint&, Error&) noexcept;

		bool is_connected() const noexcept;
		bool is_open() const noexcept;
		void close() noexcept;

		std::size_t send(const void*, std::size_t, Error&)  noexcept;
		std::size_t receive(void*, std::size_t, Error&) noexcept;
		template<int BlockTimeMs>
		std::size_t receive(void*, std::size_t, Error&) noexcept;
	private:
		handler socket_;
};

}//POSIX
}//Soca

#include "impl/tcp_client_impl.hpp"

#endif /* SOCA_POSIX_SOCKET_TCP_CLIENT_HPP__ */
