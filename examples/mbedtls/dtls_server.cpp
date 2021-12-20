/*
 *  Simple DTLS server demonstration program
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
#define mbedtls_time_t     time_t
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif

#include "dtls_server.hpp"
#include <cstring>

/* Uncomment out the following line to default to IPv4 and disable IPv6 */
//#define FORCE_IPV4

#define SERVER_PORT "4433"

#ifdef FORCE_IPV4
#define BIND_IP     "0.0.0.0"     /* Forces IPv4 */
#else
#define BIND_IP     "::"
#endif

#if !defined(MBEDTLS_SSL_SRV_C) || !defined(MBEDTLS_SSL_PROTO_DTLS) ||    \
    !defined(MBEDTLS_SSL_COOKIE_C) || !defined(MBEDTLS_NET_C) ||          \
    !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C) ||        \
    !defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_RSA_C) ||      \
    !defined(MBEDTLS_PEM_PARSE_C) || !defined(MBEDTLS_TIMING_C)

int main( void )
{
    printf( "MBEDTLS_SSL_SRV_C and/or MBEDTLS_SSL_PROTO_DTLS and/or "
            "MBEDTLS_SSL_COOKIE_C and/or MBEDTLS_NET_C and/or "
            "MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C and/or "
            "MBEDTLS_X509_CRT_PARSE_C and/or MBEDTLS_RSA_C and/or "
            "MBEDTLS_PEM_PARSE_C and/or MBEDTLS_TIMING_C not defined.\n" );
    mbedtls_exit( 0 );
}
#else

#define READ_TIMEOUT_MS 10000   /* 10 seconds */

const unsigned char psk[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

const char psk_id[] = "Client_identity";

int main( void )
{
    int ret, len;

    unsigned char buf[1024];
    const char *pers = "dtls_server";
    unsigned char client_ip[16] = { 0 };
    std::size_t cliip_len;
    std::size_t psk_len = sizeof(psk);

    /*
     * 1. Seed the RNG
     */
    printf( "  . Seeding the random number generator..." );
    fflush( stdout );

    Soca::DTLS_Server conn((const unsigned char*)pers, std::strlen(pers), ret);
    if(ret != 0)
    {
        printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }

    printf(" ok\n");

    /*
     * 3. Setup the "listening" UDP socket
     */
    printf( "  . Bind on udp/*/%s ...", SERVER_PORT);
    fflush( stdout );

    ret = conn.bind(BIND_IP, SERVER_PORT);
    if(ret != 0 )
    {
        printf( " failed\n  ! mbedtls_net_bind returned %d\n\n", ret );
        goto exit;
    }

    printf( " ok\n" );

    /*
     * 4. Setup stuff
     */
    printf( "  . Setting up the DTLS data..." );
    fflush( stdout );

    ret = conn.config(psk, psk_len, (const unsigned char*)psk_id, strlen(psk_id), READ_TIMEOUT_MS);
    if(ret != 0)
    {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        goto exit;
    }

    printf( " ok\n" );

reset:
#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        printf("Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    conn.reset();

    /*
     * 3. Wait until a client connects
     */
    printf( "  . Waiting for a remote connection ..." );
    fflush( stdout );

    cliip_len = sizeof(client_ip);
    ret = conn.listen(client_ip, cliip_len);
    if(ret != 0)
    {
        printf( " failed\n  ! mbedtls_net_accept returned %d\n\n", ret );
        goto exit;
    }

    printf( " ok\n" );

    /*
     * 5. Handshake
     */
    printf( "  . Performing the DTLS handshake..." );
    fflush( stdout );

    ret = conn.handshake();
    if(ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED)
    {
        printf(" hello verification requested\n");
        ret = 0;
        goto reset;
    }
    else if( ret != 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", (unsigned int) -ret );
        goto reset;
    }

    printf( " ok\n" );

    /*
     * 6. Read the echo Request
     */
    printf( "  < Read from client:" );
    fflush( stdout );

    len = sizeof( buf ) - 1;
    memset( buf, 0, sizeof( buf ) );

    ret = conn.read(buf, len);
    if( ret <= 0 )
    {
        switch( ret )
        {
            case MBEDTLS_ERR_SSL_TIMEOUT:
                printf( " timeout\n\n" );
                goto reset;

            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                printf( " connection was closed gracefully\n" );
                ret = 0;
                goto close_notify;

            default:
                printf( " mbedtls_ssl_read returned -0x%x\n\n", (unsigned int) -ret );
                goto reset;
        }
    }

    len = ret;
    printf( " %d bytes read\n\n%s\n\n", len, buf );

    /*
     * 7. Write the 200 Response
     */
    printf( "  > Write to client:" );
    fflush( stdout );

    ret = conn.write(buf, len);
    if( ret < 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
        goto exit;
    }

    len = ret;
    printf( " %d bytes written\n\n%s\n\n", len, buf );

    /*
     * 8. Done, cleanly close the connection
     */
close_notify:
    printf( "  . Closing the connection..." );

    /* No error checking, the connection might be closed already */
    conn.close_client();
    ret = 0;

    printf( " done\n" );

    goto reset;

    /*
     * Final clean-ups and exit
     */
exit:

#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        printf( "Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    //conn.destroy();

#if defined(_WIN32)
    printf( "  Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    /* Shell can not handle large exit numbers -> 1 for errors */
    if( ret < 0 )
        ret = 1;

    mbedtls_exit( ret );
}
#endif /* MBEDTLS_SSL_SRV_C && MBEDTLS_SSL_PROTO_DTLS &&
          MBEDTLS_SSL_COOKIE_C && MBEDTLS_NET_C && MBEDTLS_ENTROPY_C &&
          MBEDTLS_CTR_DRBG_C && MBEDTLS_X509_CRT_PARSE_C && MBEDTLS_RSA_C
          && MBEDTLS_PEM_PARSE_C && MBEDTLS_TIMING_C */
