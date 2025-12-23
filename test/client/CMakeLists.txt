# Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

set(UDP_RELAY_CLIENT_LIB ${PROJECT_NAME}-client-static)

add_library(${UDP_RELAY_CLIENT_LIB} STATIC)

target_link_libraries(${UDP_RELAY_CLIENT_LIB} ${UDP_RELAY_LIB_NAME})

target_include_directories(${UDP_RELAY_CLIENT_LIB} PUBLIC "include")

target_sources(${UDP_RELAY_CLIENT_LIB}
                PRIVATE
                    src/client.cxx
                PUBLIC
                    FILE_SET HEADERS
                    FILES
                    include/udp-relay-client/client.hxx
                PUBLIC
                    FILE_SET CXX_MODULES
                    FILES
                    # modules; later
                )

install(TARGETS ${UDP_RELAY_CLIENT_LIB}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_PREFIX})