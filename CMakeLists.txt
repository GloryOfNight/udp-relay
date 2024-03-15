cmake_minimum_required(VERSION 3.21.0)

project(udp-relay
        VERSION 1.0
        DESCRIPTION "Instanly simple and efficient UDP packet relay"
        HOMEPAGE_URL "https://github.com/GloryOfNight/clang-format-all"
        LANGUAGES CXX C)

include(cmake/ProjectOptions.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/PreventInSourceBuilds.cmake)

set (UDP_RELAY_SOURCES  "src/real_main.cxx" 
                        "src/relay/relay.cxx")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(UDP_RELAY_SOURCES   ${UDP_RELAY_SOURCES}
                            "src/unixmain.cxx"
                            "src/socket/unix/internetaddrUnix.cxx"
                            "src/socket/unix/udpsocketUnix.cxx")
    set(UDP_RELAY_MAIN      "src/unixmain.cxx")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(UDP_RELAY_SOURCES   ${UDP_RELAY_SOURCES}
                            "src/winmain.cxx"
                            "src/socket/win/internetaddrWin.cxx"
                            "src/socket/win/udpsocketWin.cxx")
    set(UDP_RELAY_MAIN      "src/winmain.cxx")
    set(UDP_RELAY_LINK_LIBRARIES ws2_32.dll)
endif()

add_executable(${PROJECT_NAME} ${UDP_RELAY_MAIN} ${UDP_RELAY_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${UDP_RELAY_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE "include")

add_library(${PROJECT_NAME}-static STATIC ${UDP_RELAY_SOURCES})
target_link_libraries(${PROJECT_NAME}-static PUBLIC ${UDP_RELAY_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}-static PUBLIC "include")


enable_sanitizers(${PROJECT_NAME} 
                    ${ENABLE_SANITIZER_ADDRESS} 
                    ${ENABLE_SANITIZER_LEAK} 
                    ${ENABLE_SANITIZER_UNDEFINED} 
                    ${ENABLE_SANITIZER_THREAD} 
                    ${ENABLE_SANITIZER_MEMORY})

install(TARGETS ${PROJECT_NAME})