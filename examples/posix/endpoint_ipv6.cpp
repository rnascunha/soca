#include "posix/endpoint_ipv6.hpp"
#include <cstdint>
#include <cstdio>

#include "error.hpp"

using namespace Soca;

int main()
{
	char	buffer[INET6_ADDRSTRLEN];
	POSIX::endpoint_ipv6 ep{IN6ADDR_LOOPBACK_INIT, 5683};

	printf("ep: [%s]:%u\n", ep.address(buffer), ep.port());

	POSIX::endpoint_ipv6 ep2;

	ep2 = ep;
	printf("ep2: [%s]:%u\n", ep2.address(buffer), ep2.port());

	if(ep == ep2)
	{
		printf("IP == IP2\n");
	}
	else
	{
		printf("IP != IP2\n");
	}

	Error ec;
	POSIX::endpoint_ipv6 ep3{"2001:0db8:85a3:0000:0000:8a2e:0370:7334", 5555, ec};

	if(ec)
	{
		printf("Error intiating IPv6 (%d %s)\n", ec.value(), ec.message());
		return 1;
	}

	printf("ep3: [%s]:%u\n", ep3.address(buffer), ep3.port());
	if(ep == ep3)
	{
		printf("IP == IP3\n");
	}
	else
	{
		printf("IP != IP3\n");
	}

	return 0;
}
