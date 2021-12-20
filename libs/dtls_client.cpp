#include "dtls_client.hpp"

namespace Soca{

DTLS_Client::DTLS_Client(const unsigned char* pers, std::size_t len, int& ret)
{
	ret = init(pers, len);
}

DTLS_Client::~DTLS_Client()
{
	destroy();
}

int DTLS_Client::init(const unsigned char* pers /* = nullptr */, std::size_t len /* = 0 */) noexcept
{
	mbedtls_net_init(&server_fd_);
	mbedtls_ssl_init(&ssl_);
	mbedtls_ssl_config_init(&conf_);
	mbedtls_ctr_drbg_init(&ctr_drbg_);
	mbedtls_entropy_init(&entropy_);

	return mbedtls_ctr_drbg_seed( &ctr_drbg_,
			mbedtls_entropy_func, &entropy_,
			pers, 0);
}

int DTLS_Client::open(const char* addr, const char* port) noexcept
{
	int ret = mbedtls_net_connect(&server_fd_, addr, port, MBEDTLS_NET_PROTO_UDP);
	if(ret != 0)
	{
		return ret;
	}

	return handshake();
}

int DTLS_Client::pre_shared_secret(const unsigned char* psk,
		std::size_t psk_len,
		const unsigned char* psk_id,
		std::size_t psk_id_len) noexcept
{
	return mbedtls_ssl_conf_psk(&conf_, psk, psk_len, (const uint8_t*)psk_id, psk_id_len);
}

int DTLS_Client::hostname(const char* name) noexcept
{
	return mbedtls_ssl_set_hostname(&ssl_, name);
}

int DTLS_Client::config(std::uint32_t timeout) noexcept
{
	int ret = mbedtls_ssl_config_defaults(&conf_,
			   MBEDTLS_SSL_IS_CLIENT,
			   MBEDTLS_SSL_TRANSPORT_DATAGRAM,
			   MBEDTLS_SSL_PRESET_DEFAULT);
	if(ret != 0)
	{
		return ret;
	}

	mbedtls_ssl_conf_rng(&conf_, mbedtls_ctr_drbg_random, &ctr_drbg_);
	mbedtls_ssl_conf_read_timeout(&conf_, timeout);

	mbedtls_ssl_set_bio(&ssl_, &server_fd_,
							 mbedtls_net_send,
							 mbedtls_net_recv,
							 mbedtls_net_recv_timeout);

	mbedtls_ssl_set_timer_cb(&ssl_, &timer_,
							mbedtls_timing_set_delay,
							mbedtls_timing_get_delay );

	return mbedtls_ssl_setup(&ssl_, &conf_);
}

int DTLS_Client::handshake() noexcept
{
	int ret;
	do ret = mbedtls_ssl_handshake(&ssl_);
	while(ret == MBEDTLS_ERR_SSL_WANT_READ ||
		  ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	return ret;
}

int DTLS_Client::write(const void* data, std::size_t len) noexcept
{
	int ret;
	do ret = mbedtls_ssl_write(&ssl_, (const unsigned char*)data, len);
	while( ret == MBEDTLS_ERR_SSL_WANT_READ ||
			ret == MBEDTLS_ERR_SSL_WANT_WRITE );

	return ret;
}

int DTLS_Client::read(void* buf, std::size_t len) noexcept
{
	int ret;
	do ret = mbedtls_ssl_read(&ssl_, (unsigned char*)buf, len);
	while(ret == MBEDTLS_ERR_SSL_WANT_READ ||
			ret == MBEDTLS_ERR_SSL_WANT_WRITE );

	return ret;
}

void DTLS_Client::close() noexcept
{
	int ret;
	do ret = mbedtls_ssl_close_notify(&ssl_);
	while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );
}

void DTLS_Client::destroy() noexcept
{
	mbedtls_net_free(&server_fd_);
	mbedtls_ssl_free(&ssl_);
	mbedtls_ssl_config_free(&conf_);
	mbedtls_ctr_drbg_free(&ctr_drbg_);
	mbedtls_entropy_free(&entropy_);
}

}//Soca

