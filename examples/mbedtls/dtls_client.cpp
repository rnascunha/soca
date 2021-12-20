/*
 *  Simple DTLS client demonstration program
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "mbedtls/build_info.h"

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_printf     printf
#define mbedtls_fprintf    fprintf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif

#if !defined(MBEDTLS_SSL_CLI_C) || !defined(MBEDTLS_SSL_PROTO_DTLS) ||    \
    !defined(MBEDTLS_NET_C)  || !defined(MBEDTLS_TIMING_C) ||             \
    !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C) ||        \
    !defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_RSA_C) ||      \
    !defined(MBEDTLS_PEM_PARSE_C)
int main( void )
{
    mbedtls_printf( "MBEDTLS_SSL_CLI_C and/or MBEDTLS_SSL_PROTO_DTLS and/or "
            "MBEDTLS_NET_C and/or MBEDTLS_TIMING_C and/or "
            "MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C and/or "
            "MBEDTLS_X509_CRT_PARSE_C and/or MBEDTLS_RSA_C and/or "
            "MBEDTLS_PEM_PARSE_C not defined.\n" );
    mbedtls_exit( 0 );
}
#else

#include "dtls_client.hpp"
#include <cstring>

#include "mbedtls/error.h"

/* Uncomment out the following line to default to IPv4 and disable IPv6 */
//#define FORCE_IPV4

#define SERVER_PORT "4433"
#define SERVER_NAME "localhost"

#ifdef FORCE_IPV4
#define SERVER_ADDR "127.0.0.1"     /* Forces IPv4 */
#else
#define SERVER_ADDR "::1"
#endif

#define MESSAGE     "Echo this"

#define READ_TIMEOUT_MS 1000
#define MAX_RETRY       5

const unsigned char psk[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
const char psk_id[] = "Client_identity";

int main( int argc, char *argv[] )
{
    int ret;
    unsigned char buf[1024];
    const char *pers = "dtls_client";
    int retry_left = MAX_RETRY;

    ((void) argc);
    ((void) argv);

    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_printf( "\n  . Seeding the random number generator..." );
    fflush( stdout );

    Soca::DTLS_Client conn((const unsigned char*)pers, std::strlen(pers), ret);
    if(ret != 0)
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
	 * 2. Setup stuff
	 */
	mbedtls_printf( "  . Setting up the DTLS structure..." );
	fflush( stdout );

	conn.pre_shared_secret(psk, sizeof(psk), (const uint8_t*)psk_id, sizeof(psk_id) - 1);
	ret = conn.config(READ_TIMEOUT_MS);
	if(ret != 0)
	{
		mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
		goto exit;
	}

	ret = conn.hostname(SERVER_NAME);
	if(ret != 0)
	{
		mbedtls_printf( " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret );
		goto exit;
	}

	mbedtls_printf(" ok\n");

    /*
     * 1. Start the connection
     */
    mbedtls_printf( "  . Connecting to udp/%s/%s...", SERVER_NAME, SERVER_PORT );
    fflush( stdout );

    ret = conn.open(SERVER_ADDR, SERVER_PORT);
    if(ret != 0)
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_connect returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 4. Handshake
     */
//    mbedtls_printf("  . Performing the DTLS handshake...");
//    fflush( stdout );
//
//    ret = conn.handshake();
//    if(ret != 0)
//    {
//        mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", (unsigned int) -ret);
//        goto exit;
//    }
//
//    mbedtls_printf(" ok\n");

    /*
     * 6. Write the echo request
     */
send_request:
    mbedtls_printf( "  > Write to server:" );
    fflush( stdout );

    ret = conn.write((unsigned char *) MESSAGE, sizeof( MESSAGE ) - 1);
    if(ret < 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " %d bytes written\n\n%s\n\n", ret, MESSAGE );

    /*
     * 7. Read the echo response
     */
    mbedtls_printf( "  < Read from server:" );
    fflush( stdout );

    memset(buf, 0, sizeof( buf ));
    ret = conn.read(buf, sizeof( buf ) - 1);
    if( ret <= 0 )
    {
        switch( ret )
        {
            case MBEDTLS_ERR_SSL_TIMEOUT:
                mbedtls_printf( " timeout\n\n" );
                if( retry_left-- > 0 )
                    goto send_request;
                goto exit;

            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                mbedtls_printf( " connection was closed gracefully\n" );
                ret = 0;
                goto close_notify;

            default:
                mbedtls_printf( " mbedtls_ssl_read returned -0x%x\n\n", (unsigned int) -ret );
                goto exit;
        }
    }

    mbedtls_printf( " %d bytes read\n\n%s\n\n", ret, buf );

    /*
     * 8. Done, cleanly close the connection
     */
close_notify:
    mbedtls_printf( "  . Closing the connection..." );

    conn.close();
    mbedtls_printf( " done\n" );

    ret = 0;
    /*
     * 9. Final clean-ups and exit
     */
exit:

#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf( "Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    //conn.destroy()	//Don't need. Already at destructor

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    /* Shell can not handle large exit numbers -> 1 for errors */
    if( ret < 0 )
        ret = 1;

    mbedtls_exit( ret );
}
#endif /* MBEDTLS_SSL_CLI_C && MBEDTLS_SSL_PROTO_DTLS && MBEDTLS_NET_C &&
          MBEDTLD_TIMING_C && MBEDTLS_ENTROPY_C && MBEDTLS_CTR_DRBG_C &&
          MBEDTLS_X509_CRT_PARSE_C && MBEDTLS_RSA_C && MBEDTLS_PEM_PARSE_C */
