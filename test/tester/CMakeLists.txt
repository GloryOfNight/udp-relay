# Copyright (c) 2025 Siarhei Dziki aka "GloryOfNight"

add_executable(${PROJECT_NAME}-tester)

target_link_libraries(${PROJECT_NAME}-tester ${PROJECT_NAME}-client-static)

target_sources(${PROJECT_NAME}-tester PRIVATE 
                        src/real_main.cxx
                        $<$<PLATFORM_ID:Linux>:src/unixmain.cxx>
                        $<$<PLATFORM_ID:Windows>:src/winmain.cxx>)

install(TARGETS ${PROJECT_NAME}-tester)