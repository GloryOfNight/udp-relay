cmake_minimum_required(VERSION 3.21.0)

project(udp-relay
        VERSION 1.0
        DESCRIPTION "Instanly simple and efficient UDP packet relay"
        HOMEPAGE_URL "https://github.com/GloryOfNight/clang-format-all"
        LANGUAGES CXX C)

include(cmake/ProjectOptions.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/PreventInSourceBuilds.cmake)

set (UDP_RELAY_SOURCES  "src/main.cxx" 
                        "src/relay/relay.cxx")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(UDP_RELAY_SOURCES   ${UDP_RELAY_SOURCES}
                            "src/unixmain.cxx"
                            "src/socket/unix/internetaddrUnix.cxx"
                            "src/socket/unix/udpsocketUnix.cxx")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(UDP_RELAY_SOURCES   ${UDP_RELAY_SOURCES}
                            "src/winmain.cxx"
                            "src/socket/win/internetaddrWin.cxx"
                            "src/socket/win/udpsocketWin.cxx")
endif()

add_executable(${PROJECT_NAME} ${UDP_RELAY_SOURCES})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.dll)
endif()



target_include_directories(${PROJECT_NAME} PRIVATE "src")

enable_sanitizers(${PROJECT_NAME} 
                    ${ENABLE_SANITIZER_ADDRESS} 
                    ${ENABLE_SANITIZER_LEAK} 
                    ${ENABLE_SANITIZER_UNDEFINED} 
                    ${ENABLE_SANITIZER_THREAD} 
                    ${ENABLE_SANITIZER_MEMORY})

install(TARGETS ${PROJECT_NAME})