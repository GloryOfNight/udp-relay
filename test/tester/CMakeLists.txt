# Copyright (c) 2025 Siarhei Dziki aka "GloryOfNight"

set(UDP_RELAY_TESTER_SOURCES    "src/real_main.cxx")

set(UDP_RELAY_TESTER_MAIN       $<$<PLATFORM_ID:Linux>:src/unixmain.cxx>
                                $<$<PLATFORM_ID:Windows>:src/winmain.cxx>)

add_executable(${PROJECT_NAME}-tester ${UDP_RELAY_TESTER_MAIN} ${UDP_RELAY_TESTER_SOURCES})
target_link_libraries(${PROJECT_NAME}-tester ${PROJECT_NAME}-client-static ${PROJECT_NAME}-static)
install(TARGETS ${PROJECT_NAME}-tester)