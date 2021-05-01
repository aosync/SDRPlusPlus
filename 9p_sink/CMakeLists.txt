cmake_minimum_required(VERSION 3.13)
project(9p_sink)

if (MSVC)
    set(CMAKE_CXX_FLAGS "-O2 /std:c++17 /EHsc")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "-O3 -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup -fPIC")
else ()
    set(CMAKE_CXX_FLAGS "-O3 -std=c++17 -fPIC -g")
endif ()

file(GLOB SRC "src/*.cpp")
include_directories("src/")

add_library(9p_sink SHARED ${SRC})
target_link_libraries(9p_sink PRIVATE sdrpp_core)
set_target_properties(9p_sink PROPERTIES PREFIX "")

# Install directives
install(TARGETS 9p_sink DESTINATION lib/sdrpp/plugins)