#ifndef SOCA_POSIX_SOCKET_UDP_HPP__
#define SOCA_POSIX_SOCKET_UDP_HPP__

#include <cstdlib>
#include <cstdint>
#include "../error.hpp"
#include "port.hpp"

namespace Soca{
namespace POSIX{

template<class Endpoint,
		int Flags = MSG_DONTWAIT>
class udp{
	public:
#if	defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		using handler = SOCKET;
#else /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
		using handler = int;
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */
		using endpoint = Endpoint;
		udp();

		void open(Error&) noexcept;
		void open(sa_family_t, Error&) noexcept;
		void open(endpoint&, Error&) noexcept;

		void bind(endpoint&, Error&) noexcept;

		void close() noexcept;

		std::size_t send(const void*, std::size_t, endpoint&, Error&)  noexcept;
		std::size_t receive(void*, std::size_t, endpoint&, Error&) noexcept;
		template<int BlockTimeMs>
		std::size_t receive(void*, std::size_t, endpoint&, Error&) noexcept;
	private:
		handler socket_;
};

}//POSIX
}//Soca

#include "impl/udp_socket_impl.hpp"

#endif /* SOCA_POSIX_SOCKET_UDP_HPP__ */
