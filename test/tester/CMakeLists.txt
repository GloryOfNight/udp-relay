# Copyright (c) 2025 Siarhei Dziki aka "GloryOfNight"

set (UDP_RELAY_TESTER_SOURCES  "src/real_main.cxx")

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(UDP_RELAY_TESTER_MAIN      "src/unixmain.cxx")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(UDP_RELAY_TESTER_MAIN      "src/winmain.cxx")
endif()

add_executable(${PROJECT_NAME}-tester ${UDP_RELAY_TESTER_MAIN} ${UDP_RELAY_TESTER_SOURCES})
target_link_libraries(${PROJECT_NAME}-tester ${PROJECT_NAME}-client-static ${PROJECT_NAME}-static)