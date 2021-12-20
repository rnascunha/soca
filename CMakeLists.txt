cmake_minimum_required (VERSION 3.10)
project (soca) 

message(STATUS "Building OS: " ${CMAKE_SYSTEM_NAME})

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

message("Builder type: " ${CMAKE_BUILD_TYPE}) 

if(MSVC)
	message(STATUS "MSVC build")
	set(CMAKE_CXX_FLAGS "/W4 /std:c++17 /EHsc /bigobj")
	set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
	message(STATUS "NO MSVC build")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -std=c++17")	#-fmax-errors=5 
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

if(WIN32)
    macro(get_WIN32_WINNT version)
        if(CMAKE_SYSTEM_VERSION)
            set(ver ${CMAKE_SYSTEM_VERSION})
            string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
            string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
            # Check for Windows 10, b/c we'll need to convert to hex 'A'.
            if("${verMajor}" MATCHES "10")
                set(verMajor "A")
                string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
            endif()
            # Remove all remaining '.' characters.
            string(REPLACE "." "" ver ${ver})
            # Prepend each digit with a zero.
            string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
            set(${version} "0x${ver}")
        endif()
    endmacro()

    get_WIN32_WINNT(ver)
    add_definitions(-D_WIN32_WINNT=${ver})
        message("WIN32: " ${ver})
endif()

message(STATUS "Compiler: " ${CMAKE_CXX_COMPILER_ID} " " ${CMAKE_CXX_COMPILER_VERSION})

message(STATUS "Finding MbedTLS...")
find_package(MbedTLS)
if(${MbedTLS_FOUND})
	message(STATUS "Found MbedTLS at ${MbedTLS_DIR}")
else()
	message(FATAL_ERROR "MbedTLS not found")
endif()

set(SOCA_DIR		libs)
set(SOCA_POSIX_DIR	${SOCA_DIR}/posix)
set(SOCA_SRC		${SOCA_DIR}/error.cpp
					${SOCA_DIR}/dtls_client.cpp 
					${SOCA_DIR}/dtls_server.cpp
					${SOCA_POSIX_DIR}/functions.cpp)

add_library(${PROJECT_NAME} STATIC ${SOCA_SRC})
target_link_libraries(${PROJECT_NAME} 
		PUBLIC 	MbedTLS::mbedtls
				MbedTLS::mbedcrypto
           		MbedTLS::mbedx509)

add_definitions(-DSOCA_PORT_POSIX=1 -DSOCA_USE_ERROR_MESSAGES=1)							

#If Windows or WebAssembly (emscripten) use SELECT implementation
if(WIN32 OR EMSCRIPTEN)
	message("Setting SELECT call implmenetation")
	add_definitions(-DSOCA_USE_SELECT=1)
endif()
         
#########################################  		
#				Examples				#
#########################################

set(EXAMPLES_DIR		examples)

set(MBEDTLS_EXAMPLE_DIR	${EXAMPLES_DIR}/mbedtls)
set(EXAMPLE_MBEDTLS_LIST	dtls_client 
							dtls_server)
foreach(example ${EXAMPLE_MBEDTLS_LIST})
	message(STATUS "Compiling MBEDTLS example ${example}...")
	add_executable(${example} ${MBEDTLS_EXAMPLE_DIR}/${example}.cpp)
	target_include_directories(${example} PRIVATE libs)
	target_link_libraries(${example} PUBLIC ${PROJECT_NAME})
endforeach()
               
set(POSIX_EXAMPLE_DIR	${EXAMPLES_DIR}/posix)
set(EXAMPLE_POSIX_LIST	async_tcp_client
						endpoint_ipv6
						tcp_client
						tcp_server
						udp_client
						udp_server)
						
foreach(example ${EXAMPLE_POSIX_LIST})
	message(STATUS "Compiling POSIX example ${example}...")
	add_executable(${example} ${POSIX_EXAMPLE_DIR}/${example}.cpp)
	target_include_directories(${example} PRIVATE libs)
	target_link_libraries(${example} PUBLIC ${PROJECT_NAME})
endforeach()			    
