/**
 * This examples shows the use of async TCP client posix-like socket, used
 * at CoAP protocol.
 *
 * We are going to implement a simple client request and wait for a answer.
 *
 * This example is implemented using IPv4 and IPv6.
 *
 * \note Before running this example, run tcp_server to open a server TCP socket
 *
 * \note The difference from the tcp_client (not async) example is the use of
 * async_open. This function will try to connect to the server, but will not block.
 * We are going to call wait_connect to check if the connection was succeful.
 *
 * \note At this example, there is no "why" to do a async connection... just showing
 * how to use the functions.
 */

#include <cstdlib>
#include <cstdio>
#include <cstdint>

#include "error.hpp"
#include "posix/tcp_client.hpp"

/**
 * Using IPv6. Commenting the following line to use IPv4
 */
#define USE_IPV6

using namespace Soca;

/**
 * Defining the endpoint
 */
#ifdef USE_IPV6
/**
 * IPv6 definitions
 */
#include "posix/endpoint_ipv6.hpp"
using endpoint = POSIX::endpoint_ipv6;
#define LOCALHOST_ADDR		"::1"
#else
/**
 * IPv4 definitions
 */
#include "posix/endpoint_ipv4.hpp"
using endpoint = POSIX::endpoint_ipv4;
#define LOCALHOST_ADDR		"127.0.0.1"
#endif /* USE_IPV6 */

/**
 * Port connect
 */
#define CONN_PORT		8080

/**
 * Auxiliary call
 */
static void exit_error(Error& ec, const char* what = "")
{
	printf("ERROR! [%d] %s [%s]\n", ec.value(), ec.message(), what);
	exit(EXIT_FAILURE);
}

#define BUFFER_LEN		1000

/**
 * Defining the TCP socket.
 *
 * The template argument is the endpoint (IPv4 or IPv6) that we are
 * going to open and connect.
 */
using tcp_client = POSIX::tcp_client<endpoint>;

int main()
{
	std::printf("Async TCP Client example init...\n");

	/**
	 * At Linux, do nothing. At Windows initiate winsock
	 */
	POSIX::init();

	Error ec;

	/**
	 * Endpoint of the host to connect
	 */
	tcp_client::endpoint to{LOCALHOST_ADDR, CONN_PORT, ec};
	if(ec)
	{
		std::printf("Error parsing address\n");
		return 1;
	}

	/**
	 * TCP client instance
	 */
	tcp_client conn;

	/**
	 * Connecting to host
	 *
	 * async_open will open a socket, configure as non-blocking and try to connect.
	 * It will return true if connection was succeful. If ec (error code) is set,
	 * a error happened at open the socket or connecting... If return false but no
	 * error code was set, the connection is in progress...
	 *
	 * wait_connect waits and check for the connection. The template paremeter means:
	 * n < 0 - blocks, i.e, wait indefinitely
	 * n = 0 -  no block. Just check if is connected
	 * n > 0 - block for 'n' miliseconds
	 *
	 * \note this example could be used syncronous functions... but we are just showing
	 * who it works...
	 */
	if(!conn.async_open(to, ec))
	{
		if(ec) exit_error(ec, "open");
		std::printf("Waiting connect...\n");
		while(!conn.wait_connect<-1>(ec))
		{
			printf(".");
			if(ec) exit_error(ec, "waiting");
		}
	}

	/**
	 * Sending a payload
	 */
	conn.send("teste", std::strlen("teste"), ec);
	if(ec) exit_error(ec, "send");
	std::printf("Send succeced!\n");

	/**
	 * Receiving buffer
	 */
	std::uint8_t buffer[BUFFER_LEN];

	/**
	 * Loop to wait for a answer
	 */
	while(true)
	{
		/**
		 * Waiting for a response
		 *
		 * The receive with a template parameter if should be a blocking
		 * read or not...
		 * n < 0 - blocks, i.e, wait indefinitely
		 * n = 0 -  no block
		 * n > 0 - blocks for 'n' miliseconds
		 */
		std::size_t size = conn.receive<-1>(buffer, BUFFER_LEN, ec);
		if(ec) exit_error(ec, "read");
		/**
		 * Printing response received
		 */
		std::printf("Received[%zu]: %.*s\n", size, static_cast<int>(size), buffer);

		break;
	}
	conn.close();

	return EXIT_SUCCESS;
}
