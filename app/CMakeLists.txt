project(bpopt)

add_executable(bpopt bpopt.c)
target_link_libraries(bpopt PRIVATE dspbptk)
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(bpopt PRIVATE pthread)
endif()
