#include "dtls_server.hpp"
#include <cstdio>

namespace Soca{

DTLS_Server::DTLS_Server(const unsigned char* pers, std::size_t len, int& ret)
{
	mbedtls_net_init(&listen_fd_);
	mbedtls_net_init(&client_fd_);
	mbedtls_ssl_init(&ssl_);
	mbedtls_ssl_config_init(&conf_);
	mbedtls_ssl_cookie_init(&cookie_ctx_);
#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_init(&cache_);
#endif

	mbedtls_entropy_init(&entropy_);
	mbedtls_ctr_drbg_init(&ctr_drbg_);

	ret = mbedtls_ctr_drbg_seed( &ctr_drbg_,
			mbedtls_entropy_func, &entropy_,
			pers, len);
}

DTLS_Server::~DTLS_Server()
{
	destroy();
}

int DTLS_Server::bind(const char* addr, const char* port) noexcept
{
	return mbedtls_net_bind(&listen_fd_, addr, port, MBEDTLS_NET_PROTO_UDP);
}

int DTLS_Server::config(const unsigned char* psk,
		std::size_t psk_len,
		const unsigned char* psk_id,
		std::size_t psk_id_len,
		unsigned int timeout) noexcept
{
	int ret = mbedtls_ssl_config_defaults(&conf_,
			MBEDTLS_SSL_IS_SERVER,
			MBEDTLS_SSL_TRANSPORT_DATAGRAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	if(ret != 0)
	{
		return ret;
	}

	mbedtls_ssl_conf_rng(&conf_, mbedtls_ctr_drbg_random, &ctr_drbg_);
	mbedtls_ssl_conf_read_timeout(&conf_, timeout);

	#if defined(MBEDTLS_SSL_CACHE_C)
		mbedtls_ssl_conf_session_cache(&conf_, &cache_,
									   mbedtls_ssl_cache_get,
									   mbedtls_ssl_cache_set);
	#endif

	ret = mbedtls_ssl_cookie_setup(&cookie_ctx_,
								  mbedtls_ctr_drbg_random, &ctr_drbg_);
	if(ret != 0)
	{
		return ret;
	}

	mbedtls_ssl_conf_dtls_cookies(&conf_,
			mbedtls_ssl_cookie_write,
			mbedtls_ssl_cookie_check,
			&cookie_ctx_);

	ret = mbedtls_ssl_conf_psk(&conf_, psk, psk_len, psk_id, psk_id_len);
	if (ret != 0)
	{
		return ret;
	}

	mbedtls_ssl_set_timer_cb(&ssl_, &timer_,
			mbedtls_timing_set_delay,
			mbedtls_timing_get_delay);

	return mbedtls_ssl_setup(&ssl_, &conf_);
}

void DTLS_Server::reset() noexcept
{
	mbedtls_net_free(&client_fd_);
	mbedtls_ssl_session_reset(&ssl_);
}

int DTLS_Server::listen(unsigned char* client_ip, std::size_t& client_ip_len) noexcept
{
	int ret = mbedtls_net_accept(&listen_fd_, &client_fd_,
			client_ip, client_ip_len, &client_ip_len);
	if(ret != 0)
	{
		return ret;
	}

	/* For HelloVerifyRequest cookies */
	ret = mbedtls_ssl_set_client_transport_id(&ssl_,
								client_ip, client_ip_len);
	if(ret != 0)
	{
		return ret;
	}

	mbedtls_ssl_set_bio(&ssl_, &client_fd_,
						 mbedtls_net_send,
						 mbedtls_net_recv,
						 mbedtls_net_recv_timeout );

	return 0;
}

int DTLS_Server::handshake() noexcept
{
	int ret;
	do ret = mbedtls_ssl_handshake(&ssl_);
	while(ret == MBEDTLS_ERR_SSL_WANT_READ ||
		  ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	return ret;
}

int DTLS_Server::read(void* buf, std::size_t len) noexcept
{
	int ret;
	do ret = mbedtls_ssl_read(&ssl_, (unsigned char*)buf, len);
	while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
		   ret == MBEDTLS_ERR_SSL_WANT_WRITE );

	return ret;
}

int DTLS_Server::write(const void* data, std::size_t size) noexcept
{
	int ret;
	do ret = mbedtls_ssl_write(&ssl_, (const unsigned char*)data, size);
	while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
		   ret == MBEDTLS_ERR_SSL_WANT_WRITE );

	return ret;
}

void DTLS_Server::close_client() noexcept
{
	while(mbedtls_ssl_close_notify(&ssl_) == MBEDTLS_ERR_SSL_WANT_WRITE);
}

void DTLS_Server::destroy() noexcept
{
	mbedtls_net_free(&client_fd_);
	mbedtls_net_free(&listen_fd_);

	mbedtls_ssl_free(&ssl_);
	mbedtls_ssl_config_free(&conf_);
	mbedtls_ssl_cookie_free(&cookie_ctx_);
#if defined(MBEDTLS_SSL_CACHE_C)
	mbedtls_ssl_cache_free(&cache_);
#endif
	mbedtls_ctr_drbg_free(&ctr_drbg_);
	mbedtls_entropy_free(&entropy_);
}

}//Soca
