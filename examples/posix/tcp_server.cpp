/**
 * This examples shows the use of TCP server posix-like socket, used
 * at CoAP protocol
 *
 * We are going to implement a simple server that will wait request from clients
 * and echo the payload received back.
 *
 * This example is implemented using IPv4 and IPv6.
 *
 * \note After running this example, run tcp_client to make the requests
 */

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <functional>

#include "error.hpp"
#include "posix/tcp_server.hpp"

/**
 * Using IPv6. Commenting the following line to use IPv4
 */
#define USE_IPV6

using namespace Soca;

/**
 * Defining the endpoint type
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
#include "coap-te/port/posix/endpoint_ipv4.hpp"
using endpoint = Port::POSIX::endpoint_ipv4;
#define BIND_ADDR		INADDR_ANY
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
using tcp_server = POSIX::tcp_server<endpoint>;

/**
 * The server works with callback function when accpet a new socket
 * (someone connects), receive data or close a connection
 */

/**
 * Open connection callback
 */
void open_cb(tcp_server::handler socket) noexcept
{
	tcp_server::endpoint ep;
	if(!ep.copy_peer_address(socket))
		return;
	char buf[46];
	printf("Opened socket [%s]:%u\n", ep.address(buf), ep.port());
}

/**
 * Close connection callback
 */
void close_cb(tcp_server::handler socket) noexcept
{
	tcp_server::endpoint ep;
	if(!ep.copy_peer_address(socket))
	{
		printf("Closed socket\n");
		return;
	}
	char buf[46];
	printf("Closed socket [%s]:%u\n", ep.address(buf), ep.port());
}

/**
 * Receving data callback
 */
bool read_cb(tcp_server::handler socket, tcp_server& conn) noexcept
{
	char buffer[BUFFER_LEN];
	Error ec;
	std::size_t size = conn.receive(socket, buffer, BUFFER_LEN, ec);
	if(ec) return false;

	tcp_server::endpoint ep;
	if(!ep.copy_peer_address(socket))
		return false;

	char buf[46];
	printf(">[%s]:%u[%zu]: %.*s\n",
			ep.address(buf), ep.port(),
			size,
			static_cast<int>(size),
			buffer);

	/**
	 * Echoing data received back
	 */
	conn.send(socket, buffer, size, ec);

	return true;
}

int main()
{
	std::printf("Echo TCP server init...\n");
	
	/**
	 * At Linux, do nothing. At Windows initiate winsock
	 */
	POSIX::init();

#if COAP_TE_USE_SELECT == 1
	std::printf("Using SELECT call...\n");
#else /* COAP_TE_USE_SELECT == 1 */
	std::printf("Using EPOLL call...\n");
#endif /* COAP_TE_USE_SELECT == 1 */

	Error ec;

	/**
	 * TCP server instance
	 */
	tcp_server conn;

	/**
	 * Endpoint to bind
	 */
	tcp_server::endpoint ep{BIND_ADDR, 8080};

	/**
	 * Open socket and binding enpoint
	 */
	conn.open(ep, ec);
	if(ec) exit_error(ec, "open");

	char addr_str[46];
	std::printf("Listening: [%s]:%u\n", ep.address(addr_str), ep.port());

	/**
	 * Working loop
	 *
	 * The run function template parameter are:
	 * * read callback
	 * * block time in miliseconds: 0 (no block), -1 (blocks indefinitely)
	 * * open connection callback
	 * * close connection callback
	 * * max event permited (ommited, defaulted to 32)
	 */
	while(conn.run<-1>(ec, std::bind(read_cb, std::placeholders::_1, conn), open_cb, close_cb))
	{
		/**
		 * Your code
		 */
	}

	if(ec) exit_error(ec, "run");
	return EXIT_SUCCESS;
}

