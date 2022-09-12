cmake_minimum_required(VERSION 3.22)
project(filter CXX C)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)
link_directories(lib)

add_library(filter SHARED src/dllmain.cpp src/resource.h src/patch.rc
        src/overlay/i_d3d9device.cpp src/overlay/i_d3d9.cpp src/subtitle.cpp include/subtitle.h include/strsub.h include/common.h)
add_executable(loader src/load.c)
target_link_libraries(filter detours d3d9 dxguid ass)
target_link_libraries(loader filter)
if (CMAKE_BUILD_TYPE AND ((CMAKE_BUILD_TYPE MATCHES "Debug") OR (CMAKE_BUILD_TYPE MATCHES "Deb-Clang")))
    add_definitions(/DDEBUG_LOG)
endif()
add_definitions(/D_USING_V110_SDK71_)
add_definitions(/DWIN32_LEAN_AND_MEAN)
add_definitions(/D_WIN32_WINNT=0x0601)
set_target_properties(filter PROPERTIES LINK_FLAGS
        "/export:DetourFinishHelperProcess,@1,NONAME /export:StartExecutable,@2")
set_target_properties(loader PROPERTIES LINK_FLAGS /subsystem:windows,\"6.01\")
