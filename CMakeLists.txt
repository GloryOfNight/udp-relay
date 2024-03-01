cmake_minimum_required(VERSION 3.21.0)

project(udp-relay
        VERSION 1.0
        DESCRIPTION "Instanly simple and efficient UDP packet relay"
        HOMEPAGE_URL "https://github.com/GloryOfNight/clang-format-all"
        LANGUAGES CXX C)

include(cmake/ProjectOptions.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/PreventInSourceBuilds.cmake)

add_executable(${PROJECT_NAME} "src/main.cxx")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

enable_sanitizers(${PROJECT_NAME} 
                    ${ENABLE_SANITIZER_ADDRESS} 
                    ${ENABLE_SANITIZER_LEAK} 
                    ${ENABLE_SANITIZER_UNDEFINED} 
                    ${ENABLE_SANITIZER_THREAD} 
                    ${ENABLE_SANITIZER_MEMORY})

install(TARGETS ${PROJECT_NAME})