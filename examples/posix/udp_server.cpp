/**
 * This examples shows the use of UDP posix-like socket, used
 * at CoAP protocol
 *
 * We are going to implement a simple server that will wait request from clients
 * and echo the payload received back.
 *
 * This example is implemented using IPv4 and IPv6.
 *
 * \note After running this example, run udp_client to make the requests
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
#define BIND_ADDR		IN6ADDR_ANY_INIT
#else
/**
 * IPv4 definitions
 */
#include "posix/endpoint_ipv4.hpp"
using endpoint = POSIX::endpoint_ipv4;
#define BIND_ADDR		INADDR_ANY
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
	
	udp_socket::endpoint ep{BIND_ADDR, 8080};

	udp_socket conn;

	conn.open(ec);
	if(ec) exit_error(ec, "open");

	conn.bind(ep, ec);
	if(ec) exit_error(ec, "bind");

	char addr_str[46];
	std::printf("Listening: [%s]:%u\n", ep.address(addr_str), ep.port());
	while(true)
	{
		udp_socket::endpoint recv_addr;
		std::size_t size = conn.receive(buffer, BUFFER_LEN, recv_addr, ec);
		if(ec) exit_error(ec, "read");
		if(size == 0) continue;
		buffer[size] = '\0';

		char addr_str2[46];
		std::printf("Received [%s]:%u [%zu]: %s\n", recv_addr.address(addr_str2), recv_addr.port(), size, buffer);
		std::printf("Echoing...\n");
		conn.send(buffer, size, recv_addr, ec);
		if(ec) exit_error(ec, "write");
	}

	return EXIT_SUCCESS;
}

