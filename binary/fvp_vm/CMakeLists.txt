cmake_minimum_required(VERSION 3.22)
project(fvp_vm)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)
aux_source_directory(src SOURCE)

add_executable(fvp_vm ${SOURCE})
