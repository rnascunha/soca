#ifndef SOCA_DTLS_SERVER_HPP__
#define SOCA_DTLS_SERVER_HPP__

#if defined(_WIN32)
#include <windows.h>
#endif

#include <cstdlib>
#include <cstdint>

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"

#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif

namespace Soca{

class DTLS_Server{
	public:
		DTLS_Server(const unsigned char* pers, std::size_t len, int& ret);
		~DTLS_Server();

		int bind(const char* addr, const char* port) noexcept;
		int config(const unsigned char* psk,
				std::size_t psk_len,
				const unsigned char* psk_id,
				std::size_t psk_id_len,
				unsigned int timeout) noexcept;

		void reset() noexcept;
		int listen(unsigned char* client_ip, std::size_t& client_ip_len) noexcept;
		int handshake() noexcept;
		int read(void* buf, std::size_t len) noexcept;
		int write(const void* data, std::size_t size) noexcept;
		void close_client() noexcept;
		void destroy() noexcept;
	private:
		mbedtls_net_context listen_fd_,
							client_fd_;
		mbedtls_ssl_cookie_ctx cookie_ctx_;
		mbedtls_entropy_context entropy_;
	    mbedtls_ctr_drbg_context ctr_drbg_;
	    mbedtls_ssl_context ssl_;
	    mbedtls_ssl_config conf_;
	    mbedtls_timing_delay_context timer_;
	#if defined(MBEDTLS_SSL_CACHE_C)
	    mbedtls_ssl_cache_context cache_;
	#endif
};

}//Soca

#endif /* SOCA_DTLS_SERVER_HPP__ */
