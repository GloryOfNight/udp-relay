# Copyright (c) 2025 Siarhei Dziki aka "GloryOfNight"

cmake_minimum_required(VERSION 3.28.0)

project(udp-relay
        VERSION 0.6.1
        DESCRIPTION "Simple UDP packet relay"
        HOMEPAGE_URL "https://github.com/GloryOfNight/udp-relay"
        LANGUAGES CXX C)

set(UDP_RELAY_LIB_NAME ${PROJECT_NAME}-static)
set(UDP_RELAY_EXE_NAME ${PROJECT_NAME}) 

include(cmake/ProjectDefaults.cmake)
include(cmake/ProjectOptions.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/PreventInSourceBuilds.cmake)

add_library(${UDP_RELAY_LIB_NAME} STATIC)

target_sources(${UDP_RELAY_LIB_NAME} PRIVATE
                    src/udp-relay/relay.cxx
                    src/udp-relay/version.cxx
                    src/udp-relay/net/udpsocket.cxx
                    src/udp-relay/net/internetaddr.cxx)

target_link_libraries(${UDP_RELAY_LIB_NAME} PUBLIC $<$<PLATFORM_ID:Windows>:ws2_32.dll>)

target_include_directories(${UDP_RELAY_LIB_NAME} PUBLIC    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                                            $<INSTALL_INTERFACE:include>)

target_compile_definitions(${UDP_RELAY_LIB_NAME} PUBLIC    UR_BUILD_DEBUG=$<CONFIG:Debug>
                                                            UR_BUILD_RELEASE=$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>
                                                            UR_PLATFORM_WINDOWS=$<PLATFORM_ID:Windows>
                                                            UR_PLATFORM_LINUX=$<PLATFORM_ID:Linux>
                                                            UR_PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
                                                            UR_PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
                                                            UR_PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH})

install(TARGETS ${UDP_RELAY_LIB_NAME} EXPORT ${PROJECT_NAME}Targets)
install(EXPORT ${PROJECT_NAME}Targets DESTINATION cmake)
install(DIRECTORY include DESTINATION include)

if (ENABLE_BUILD_EXEC)
    add_executable(${UDP_RELAY_EXE_NAME})
    target_sources(${UDP_RELAY_EXE_NAME} PRIVATE 
                        src/real_main.cxx
                        $<$<PLATFORM_ID:Linux>:src/unixmain.cxx>
                        $<$<PLATFORM_ID:Windows>:src/winmain.cxx>)
    target_link_libraries(${UDP_RELAY_EXE_NAME} PRIVATE ${UDP_RELAY_LIB_NAME})
    install(TARGETS ${UDP_RELAY_EXE_NAME})
endif()

if (ENABLE_BUILD_TEST)
    add_subdirectory(test)
endif()

enable_sanitizers(${PROJECT_NAME} 
                    ${ENABLE_SANITIZER_ADDRESS} 
                    ${ENABLE_SANITIZER_LEAK} 
                    ${ENABLE_SANITIZER_UNDEFINED} 
                    ${ENABLE_SANITIZER_THREAD} 
                    ${ENABLE_SANITIZER_MEMORY})

# Write .gitignore file ignoring all contents in the build dir
file (WRITE ${CMAKE_BINARY_DIR}/.gitignore "*")