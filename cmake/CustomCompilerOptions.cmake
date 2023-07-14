macro(fix_compile_flags)
    if(MSVC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    endif()
endmacro()

macro(fix_release_flags)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s -flto")
        set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -s -flto")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -s -flto")
        set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} -s -flto")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto -pipe -march=native -mtune=native")
    endif()
    if(MSVC)
        add_compile_options(
            $<$<CONFIG:Release>:/GL>
            $<$<CONFIG:MinSizeRel>:/GL>
        )
        add_link_options(
            $<$<CONFIG:Release>:/LTCG>
            $<$<CONFIG:MinSizeRel>:/LTCG>
        )
    endif()
endmacro()

macro(add_static_runtime_option)
    option(USE_STATIC_CRT "Use static C runtime" OFF)

    if(USE_STATIC_CRT)
        if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static")
        elseif(MSVC)
            set(CompilerFlags
                CMAKE_C_FLAGS
                CMAKE_C_FLAGS_DEBUG
                CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL
                CMAKE_C_FLAGS_RELWITHDEBINFO
                )
            foreach(CompilerFlag ${CompilerFlags})
                string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
                set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
            endforeach()
            add_compile_options(
                $<$<CONFIG:>:/MT>
                $<$<CONFIG:Debug>:/MTd>
                $<$<CONFIG:Release>:/MT>
                $<$<CONFIG:MinSizeRel>:/MT>
                $<$<CONFIG:RelWithDebInfo>:/MT>
            )
        endif()
    endif()
endmacro()
