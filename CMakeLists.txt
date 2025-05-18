cmake_minimum_required(VERSION 3.30)
project(powermetre C)

set(CMAKE_C_STANDARD 23)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

link_libraries(m)
add_executable(powermetre main.c
)
target_link_libraries(powermetre PUBLIC papi)