cmake_minimum_required(VERSION 4.4)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(RLOption
    VERSION 0.0.1
    LANGUAGES CXX
    DESCRIPTION ""
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(a.out)

set(RLOPTION_COMPILE_OPTIONS
    -Werror
    -Wpadded
    -g
    -fno-omit-frame-pointer
    -D_GLIBCXX_ASSERTIONS
    -D_GLIBCXX_DEBUG
    -std=c++26
    -fsanitize=address,undefined
    -O3
    -flto
    -march=native
)

set(RLOPTION_LINK_OPTIONS
    -fsanitize=address,undefined
    -static-libstdc++
    -lstdc++exp
    -O3
    -flto
    -DNDEBUG
    -march=native
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND RLOPTION_COMPILE_OPTIONS
        -Weverything
        -Wno-unsafe-buffer-usage
        -Wno-lifetime-safety
        -Wno-lifetime-safety-all
        -Wno-lifetime-safety-permissive
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-c++98-compat-local-type-template-args
        -Wno-c99-designator
        -Wno-gnu-case-range
        -Wno-pre-c++14-compat
        -Wno-pre-c++14-compat-pedantic
        -Wno-pre-c++17-compat
        -Wno-pre-c++17-compat-pedantic
        -Wno-pre-c++20-compat
        -Wno-pre-c++20-compat-pedantic
        -Wno-pre-c++23-compat
        -Wno-pre-c++23-compat-pedantic
        -Wno-pre-c++26-compat
        -Wno-pre-c++26-compat-pedantic
        -Wno-switch-default
        -Wno-covered-switch-default
        -Wno-unused-command-line-argument
        -Wno-weak-vtables
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-missing-prototypes
        -Wno-nrvo
        -fsafe-buffer-usage-suggestions
        -ferror-limit=999999
        -fcolor-diagnostics
        -fsanitize=local-bounds
        -stdlib=libstdc++
    )
    list(APPEND RLOPTION_LINK_OPTIONS -static-libsan)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND RLOPTION_COMPILE_OPTIONS
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wold-style-cast
        -Wcast-qual
        -Woverloaded-virtual
        -Wnon-virtual-dtor
        -Wnull-dereference
        -Wformat=2
        -Wundef
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
        -Wdouble-promotion
        -fmax-errors=999999
        -fdiagnostics-color=always
    )
    # The GNU dynamic sanitizer runtimes are used so this also works when the
    # toolchain does not install libasan.a or libubsan.a.
else()
    message(FATAL_ERROR
        "RLOption's strict build flags support Clang and GNU C++ only")
endif()

target_compile_options(a.out PRIVATE ${RLOPTION_COMPILE_OPTIONS})

target_sources(a.out PUBLIC
    src/Option.cpp
)

target_include_directories(a.out PRIVATE
    include/
)

target_link_options(a.out PRIVATE ${RLOPTION_LINK_OPTIONS})

target_link_libraries(a.out PRIVATE -lstdc++exp)

option(RLOPTION_BUILD_TEST_SUITE
       "Build every standalone C++ source in tests/ with the a.out flags"
       OFF)

set(RLOPTION_MATCH_ARM_COUNT 256 CACHE STRING
    "Number of generated arms in match_arm_volume.cpp")
if(NOT RLOPTION_MATCH_ARM_COUNT MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "RLOPTION_MATCH_ARM_COUNT must be a positive integer")
endif()

if(RLOPTION_BUILD_TEST_SUITE)
    enable_testing()
    file(GLOB RLOPTION_TEST_SOURCES CONFIGURE_DEPENDS
         "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    set(RLOPTION_TEST_TARGETS)

    foreach(test_source IN LISTS RLOPTION_TEST_SOURCES)
        get_filename_component(test_name "${test_source}" NAME_WE)
        set(test_target "test_${test_name}")
        add_executable(${test_target} "${test_source}")

        # Keep every suite build on the exact compiler, include, compile, and
        # link options used by the project's primary CMake target.
        target_compile_options(
            ${test_target} PRIVATE
            $<TARGET_PROPERTY:a.out,COMPILE_OPTIONS>)
        target_include_directories(
            ${test_target} PRIVATE
            $<TARGET_PROPERTY:a.out,INCLUDE_DIRECTORIES>)
        target_link_options(
            ${test_target} PRIVATE
            $<TARGET_PROPERTY:a.out,LINK_OPTIONS>)
        target_link_libraries(
            ${test_target} PRIVATE
            $<TARGET_PROPERTY:a.out,LINK_LIBRARIES>)

        if(test_name STREQUAL "match_arm_volume")
            target_compile_definitions(
                ${test_target} PRIVATE
                RLOPTION_MATCH_ARM_COUNT=${RLOPTION_MATCH_ARM_COUNT})

        endif()

        list(APPEND RLOPTION_TEST_TARGETS ${test_target})

        # This target intentionally aborts to verify that reentrant mutation
        # is rejected; run it separately and check for its expected signal.
        if(NOT test_name STREQUAL "storage_reentrancy_rejected_mutation")
            add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_target}>)
        endif()
    endforeach()

    add_custom_target(test-suites DEPENDS ${RLOPTION_TEST_TARGETS})
endif()
