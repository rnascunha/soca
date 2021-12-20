#ifndef SOCA_DTLS_CLIENT_HPP__
#define SOCA_DTLS_CLIENT_HPP__

#include <cstdint>

#include "mbedtls/net_sockets.h"
//#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/timing.h"

namespace Soca{

class DTLS_Client{
	public:
		DTLS_Client(const unsigned char* pers, std::size_t len, int& ret);
		~DTLS_Client();

		int pre_shared_secret(const unsigned char* psk,
				std::size_t psk_len,
				const unsigned char* psk_id,
				std::size_t psk_id_len) noexcept;
		int hostname(const char* name) noexcept;
		int config(std::uint32_t timeout) noexcept;

		int open(const char* addr, const char* port) noexcept;
		void close() noexcept;

		int write(const void* data, std::size_t len) noexcept;
		int read(void* buf, std::size_t len) noexcept;
	private:
		int init(const unsigned char* pers = nullptr, std::size_t len = 0) noexcept;
		void destroy() noexcept;

		int handshake() noexcept;

		mbedtls_net_context server_fd_;
		mbedtls_entropy_context entropy_;
	    mbedtls_ctr_drbg_context ctr_drbg_;
	    mbedtls_ssl_context ssl_;
	    mbedtls_ssl_config conf_;
	    mbedtls_timing_delay_context timer_;
};

}//Soca

#endif /* SOCA_DTLS_CLIENT_HPP__ */
