#ifndef SOCA_PORT_HPP__
#define SOCA_PORT_HPP__

#if SOCA_PORT_POSIX == 1
#include "posix/port.hpp"
#endif /* SOCA_PORT_POSIX == 1 */
#if SOCA_PORT_ESP_MESH == 1
#include "esp_mesh/port.hpp"
#endif /* SOCA_PORT_POSIX == 1 */
#if SOCA_PORT_ESP_MESH != 1 && SOCA_PORT_POSIX != 1
#error "System not defined"
#endif

namespace Soca{

/**
 * \brief Initalize system requiriments
 *
 * Window/Linux: Initialize random number generator
 * Windows: initialize winsock library
 */
void init() noexcept;

}//Soca

#endif /* SOCA_PORT_HPP__ */
