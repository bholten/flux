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

# LCL submodule configuration
set(LCL_BUILD_CURL ON CACHE BOOL "Build LCL CURL extension for Flux" FORCE)
set(LCL_BUILD_IO ON CACHE BOOL "Build LCL IO extension for Flux" FORCE)
set(LCL_BUILD_JSON ON CACHE BOOL "Build LCL JSON extension for Flux" FORCE)
set(LCL_BUILD_CRYPTO ON CACHE BOOL "Build LCL Crypto extension for Flux" FORCE)
set(LCL_BUILD_TIME ON CACHE BOOL "Build Lcl time extension" FORCE)
set(LCL_BUILD_REGEX ON CACHE BOOL "Build Lcl regex extension" FORCE)
set(LCL_BUILD_CLI OFF CACHE BOOL "Don't build LCL CLI" FORCE)
set(LCL_BUILD_SHARED OFF CACHE BOOL "Don't build shared LCL" FORCE)

add_subdirectory(submodules/lcl)

if(FLUX_ENABLE_ASAN)
  list(APPEND FLUX_COMPILE_OPTIONS -fsanitize=address,undefined -fno-omit-frame-pointer)
  set(FLUX_LINK_OPTIONS -fsanitize=address,undefined)
endif()

# Generate embedded DSL header from lib/flux.lcl
set(DSL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lib/flux.lcl)
set(DSL_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/generated/flux_dsl.h)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/generated)

add_custom_command(
  OUTPUT ${DSL_HEADER}
  COMMAND xxd -i lib/flux.lcl > ${DSL_HEADER}.tmp
  COMMAND sed -i -e "s/unsigned char/static const unsigned char/" -e "s/unsigned int/static const unsigned int/" -e "s/lib_flux_lcl/flux_dsl_lcl/g" ${DSL_HEADER}.tmp
  # Add null terminator: xxd doesn't include one, but we need it for C strings
  COMMAND sed -i -e "s/}\\;/, 0x00}\\;/" ${DSL_HEADER}.tmp
  COMMAND ${CMAKE_COMMAND} -E rename ${DSL_HEADER}.tmp ${DSL_HEADER}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${DSL_SOURCE}
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
  ${CMAKE_CURRENT_SOURCE_DIR}/submodules/lcl/include
  ${CMAKE_CURRENT_SOURCE_DIR}/submodules/lcl/packages/lcl-io/include
  ${CMAKE_CURRENT_SOURCE_DIR}/submodules/lcl/packages/lcl-curl/include
  ${CMAKE_CURRENT_SOURCE_DIR}/submodules/lcl/packages/lcl-json/include
  ${CMAKE_CURRENT_SOURCE_DIR}/submodules/lcl/packages/lcl-crypto/include
  ${CMAKE_CURRENT_SOURCE_DIR}/submodules/lcl/packages/lcl-time/include
)

target_link_libraries(flux PRIVATE lcl_static lcl_io lcl_curl lcl_json lcl_crypto lcl_time)

add_dependencies(flux generate_dsl_header)

if(FLUX_ENABLE_ASAN)
  target_link_options(flux PRIVATE ${FLUX_LINK_OPTIONS})
endif()

install(TARGETS flux RUNTIME DESTINATION bin)
