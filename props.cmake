option(NO_CONSTEXPR "Do not use constexpr in teh library" NO)
option(TRACE "Trace the execution of teh library" NO)

function(set_props exe)

    set(gcc_warnings -pedantic -Wall -Wextra -Wshadow -Wnon-virtual-dtor
            -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic
            -Wconversion -Wsign-conversion -Wmisleading-indentation
            -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference
            -Wuseless-cast -Wdouble-promotion -Wformat=2 -Weffc++ -Werror)

    set(clang_warnings ${gcc_warnings} -Weverything -Wno-c++98-compat -Wno-padded
            -Wno-missing-prototypes -Wno-unknown-warning-option )

    set(msvc_warnings /W4 /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296
            /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826
            /w14905 /w14906 /w14928 /permissive- /Wx)

    set(msvc_flags)
    set(gcc_flags)
    set(clang_flags ${gcc_flags})

    if(MSVC)
        target_compile_options(${exe} PRIVATE ${msvc_warnings} ${msvc_flags})
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${exe} PRIVATE ${gcc_warnings} ${gcc_flags})
    else()
        target_compile_options(${exe} PRIVATE ${clang_warnings} ${clang_flags})
    endif()


    if(NO_CONSTEXPR)
        target_compile_definitions(${exe} PRIVATE CTPEG_NO_CONSTEXPR)
    endif()
    if(TRACE)
        target_compile_definitions(${exe} PRIVATE CTPEG_TRACE)
    endif()
    target_include_directories(${exe} PRIVATE ${CMAKE_SOURCE_DIR}/deps/expected/include)
endfunction()