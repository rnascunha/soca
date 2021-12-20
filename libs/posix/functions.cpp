#ifndef SOCA_POSIX_FUNCTIONS_HPP__
#define SOCA_POSIX_FUNCTIONS_HPP__

#include "functions.hpp"
#include "port.hpp"

namespace Soca{
namespace POSIX{

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
bool init() noexcept
{
	WSADATA wsa;

	//Initialise winsock
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		return false;
	}
	return true;
}
#endif /* defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) */

}//POSIX
}//Soca

#endif /* SOCA_POSIX_FUNCTIONS_HPP__ */
