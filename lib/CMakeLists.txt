project(dspbptk)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(LIBDEFLATE_BUILD_GZIP OFF CACHE BOOL "Build the libdeflate-gzip program" FORCE)
set(LIBDEFLATE_BUILD_TESTS OFF CACHE BOOL "Build the test programs" FORCE)
set(LIBDEFLATE_BUILD_SHARED_LIB OFF CACHE BOOL "Build the shared library" FORCE)
add_subdirectory(libdeflate)
add_subdirectory(Turbo-Base64)
set(DSPBPTK_SRC enum_offset.h libdspbptk.c libdspbptk.h md5f.c md5f.h)
add_library(dspbptk STATIC ${DSPBPTK_SRC})
target_link_libraries(dspbptk PUBLIC libdeflate::libdeflate_static base64)
target_include_directories(dspbptk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(dspbptk PROPERTIES PUBLIC_HEADER "libdspbptk.h")
install(TARGETS dspbptk
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include)
