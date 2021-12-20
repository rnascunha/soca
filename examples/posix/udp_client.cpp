/**
 * This examples shows the use of UDP posix-like socket, used
 * at CoAP protocol
 *
 * We are going to implement a simple client request and wait for a answer.
 *
 * This example is implemented using IPv4 and IPv6.
 *
 * \note Before running this example, run udp_server to open a server UDP socket
 */

#include <cstdlib>
#include <cstdio>
#include <cstdint>

#include "error.hpp"
#include "posix/udp_socket.hpp"

/**
 * Using IPv6. Commenting the following line to use IPv4
 */
//#define USE_IPV6

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
 * Auxiliary call
 */
static void exit_error(Error& ec, const char* what = "")
{
	printf("ERROR! [%d] %s [%s]", ec.value(), ec.message(), what);
	exit(EXIT_FAILURE);
}

#define BUFFER_LEN		1000

/**
 * Defining the UDP socket.
 *
 * The fisrt argument is the endpoint (IPv4 or IPv6) that we are
 * going to open and connect.
 * The seconds is a socket receiving flag, instructing the socket
 * not to block (as must be at CoAP-te);
 * The last is the socket send flag (no flag).
 */
using udp_socket = POSIX::udp<endpoint>;

int main()
{
	/**
	 * At Linux, do nothing. At Windows initiate winsock
	 */
	POSIX::init();

	Error ec;
	std::uint8_t buffer[BUFFER_LEN];
	
	const char* payload = "Teste";
	std::size_t payload_len = std::strlen(payload);

	std::memcpy(buffer, payload, payload_len + 1);

	udp_socket::endpoint to{LOCALHOST_ADDR, 8080, ec};
	if(ec)
	{
		printf("Error parsing address\n");
		return 1;
	}

	udp_socket conn;

	conn.open(ec);
	if(ec) exit_error(ec, "open");

	conn.send(buffer, payload_len, to, ec);
	if(ec) exit_error(ec, "send");
	printf("Send succeced!\n");

	udp_socket::endpoint from;
	while(true)
	{
		std::size_t size = conn.receive(buffer, BUFFER_LEN, from, ec);
		if(ec) exit_error(ec, "read");
		if(size == 0)
		{
//			std::printf(".");
			continue;
		}

		char addr_str[46];
		std::printf("Received [%s]:%u [%zu]: %s\n", from.address(addr_str), from.port(), size, buffer);
		break;
	}
	return EXIT_SUCCESS;
}
