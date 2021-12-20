/**
 * This examples shows the use of TCP client posix-like socket, used
 * at CoAP protocol
 *
 * We are going to implement a simple client request and wait for a answer.
 *
 * This example is implemented using IPv4 and IPv6.
 *
 * \note Before running this example, run tcp_server to open a server TCP socket
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
using endpoint = Port::POSIX::endpoint_ipv4;
#define LOCALHOST_ADDR		"127.0.0.1"
#endif /* USE_IPV6 */

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
	printf("TCP Client example init...\n");

	/**
	 * At Linux, do nothing. At Windows initiate winsock
	 */
	POSIX::init();
	
	Error ec;

	/**
	 * Endpoint of the host to connect
	 */
	tcp_client::endpoint to{LOCALHOST_ADDR, 8080, ec};
	if(ec)
	{
		printf("Error parsing address\n");
		return 1;
	}

	/**
	 * TCP client instance
	 */
	tcp_client conn;

	/**
	 * Connecting to host
	 */
	conn.open(to, ec);
	if(ec) exit_error(ec, "open");

	/**
	 * Sending a payload
	 */
	conn.send("teste", std::strlen("teste"), ec);
	if(ec) exit_error(ec, "send");
	printf("Send succeced!\n");

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
		 */
		std::size_t size = conn.receive(buffer, BUFFER_LEN, ec);
		if(ec) exit_error(ec, "read");
		if(size == 0)
		{
			continue;
		}

		/**
		 * Printing response received
		 */
		std::printf("Received[%zu]: %.*s\n", size, static_cast<int>(size), buffer);

		break;
	}
	conn.close();

	return EXIT_SUCCESS;
}
