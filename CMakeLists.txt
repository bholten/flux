cmake_minimum_required(VERSION 3.14)

project(flux
  VERSION 0.1.0
  DESCRIPTION "Flux HTTP testing framework"
  LANGUAGES C
)

option(FLUX_ENABLE_ASAN "Enable AddressSanitizer (debug builds)" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(FLUX_COMPILE_OPTIONS -Wall -Wextra -D_POSIX_C_SOURCE=200809L)

# Lcl dependency (fetched at configure time).
set(LCL_BUILD_CURL ON CACHE BOOL "Build LCL CURL extension for Flux" FORCE)
set(LCL_BUILD_IO ON CACHE BOOL "Build LCL IO extension for Flux" FORCE)
set(LCL_BUILD_JSON ON CACHE BOOL "Build LCL JSON extension for Flux" FORCE)
set(LCL_BUILD_CRYPTO ON CACHE BOOL "Build LCL Crypto extension for Flux" FORCE)
set(LCL_BUILD_TIME ON CACHE BOOL "Build Lcl time extension" FORCE)
set(LCL_BUILD_REGEX ON CACHE BOOL "Build Lcl regex extension" FORCE)
set(LCL_BUILD_CLI OFF CACHE BOOL "Don't build LCL CLI" FORCE)
set(LCL_BUILD_SHARED OFF CACHE BOOL "Don't build shared LCL" FORCE)

include(FetchContent)
FetchContent_Declare(
  lcl
  GIT_REPOSITORY https://github.com/bholten/lcl.git
  GIT_TAG        6b0b09537a20516103363a05c9cf37c8aa7734d4
)
FetchContent_MakeAvailable(lcl)

if(FLUX_ENABLE_ASAN)
  list(APPEND FLUX_COMPILE_OPTIONS -fsanitize=address,undefined -fno-omit-frame-pointer)
  set(FLUX_LINK_OPTIONS -fsanitize=address,undefined)
endif()

# Generate embedded DSL header from lib/flux.lcl.
# Uses a small CMake helper (no xxd dependency); output is source-
# compatible with `xxd -i lib/flux.lcl`.
set(DSL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lib/flux.lcl)
set(DSL_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/generated/flux.h)
set(DSL_EMBED_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_lib.cmake)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/generated)

add_custom_command(
  OUTPUT ${DSL_HEADER}
  COMMAND ${CMAKE_COMMAND}
    -DINPUT=${DSL_SOURCE}
    -DOUTPUT=${DSL_HEADER}
    -DSYMBOL=lib_flux_lcl
    -P ${DSL_EMBED_SCRIPT}
  DEPENDS ${DSL_SOURCE} ${DSL_EMBED_SCRIPT}
  COMMENT "Generating embedded DSL header from lib/flux.lcl"
)

add_custom_target(generate_dsl_header DEPENDS ${DSL_HEADER})

# Flux executable
set(FLUX_SOURCES
  src/lcl_interp.c
  src/main.c
)

add_executable(flux ${FLUX_SOURCES})

target_compile_options(flux PRIVATE ${FLUX_COMPILE_OPTIONS})

target_include_directories(flux PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/generated
)

target_link_libraries(flux PRIVATE lcl_static lcl_io lcl_curl lcl_json lcl_crypto lcl_time lcl_regex)

add_dependencies(flux generate_dsl_header)

if(FLUX_ENABLE_ASAN)
  target_link_options(flux PRIVATE ${FLUX_LINK_OPTIONS})
endif()

install(TARGETS flux RUNTIME DESTINATION bin)

# Test suite — runs Flux against itself via the docker-compose echo
# server (start it first: `docker compose up -d`). Each test/*.lcl
# file becomes one ctest case; `ctest -j` runs them in parallel.
enable_testing()

file(GLOB FLUX_TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test/*.lcl)
foreach(test_file ${FLUX_TEST_FILES})
  get_filename_component(test_name ${test_file} NAME_WE)
  add_test(
    NAME flux_${test_name}
    COMMAND $<TARGET_FILE:flux> ${CMAKE_CURRENT_SOURCE_DIR}/${test_file}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endforeach()
