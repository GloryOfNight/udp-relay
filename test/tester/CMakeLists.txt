# Copyright (c) 2025 Siarhei Dziki aka "GloryOfNight"

add_executable(${PROJECT_NAME}-tester)

target_link_libraries(${PROJECT_NAME}-tester ${UDP_RELAY_LIB_NAME})

target_sources(${PROJECT_NAME}-tester
                PRIVATE
                    src/tester_main.cxx
                    src/client.cxx
                PUBLIC
                    FILE_SET HEADERS
                    FILES
                    include/client.hxx
                PUBLIC
                    FILE_SET CXX_MODULES
                    FILES
                    # modules; later
                )

target_include_directories(${PROJECT_NAME}-tester PUBLIC include)

install(TARGETS ${PROJECT_NAME}-tester)