# Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

add_library(${PROJECT_NAME}-client-static STATIC "src/client.cxx")
target_link_libraries(${PROJECT_NAME}-client-static ${PROJECT_NAME}-static)
target_include_directories(${PROJECT_NAME}-client-static PUBLIC "include")