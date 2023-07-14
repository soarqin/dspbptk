project(bpopt)

add_executable(bpopt bpopt.c)
target_link_libraries(bpopt PRIVATE dspbptk)
