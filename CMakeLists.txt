cmake_minimum_required(VERSION 3.1...3.27)

project(jlm
        VERSION 1.0
        LANGUAGES CXX C)

# The LLVM libraries linked against
set(LLVM_LIBS core irReader)

# =================================================
# Finding and using llvm-config for LLVM
# =================================================
if(NOT DEFINED LLVMCONFIG)
  message(STATUS "LLVMCONFIG not specified, looking for LLVMConfig.cmake")
  find_package(LLVM REQUIRED CONFIG)
  message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

  set(LLVMCONFIG "${LLVM_TOOLS_BINARY_DIR}/llvm-config")
endif()

# Make sure the used value for llvm-config gets cached
set(LLVMCONFIG "${LLVMCONFIG}" CACHE STRING "Path to llvm-config" FORCE)

# Check the llvm-config version
execute_process(COMMAND ${LLVMCONFIG} --version OUTPUT_VARIABLE LLVM_VERSION)
if(NOT (LLVM_VERSION MATCHES "^15\."))
  message(FATAL_ERROR "Only LLVM 15 is supported!")
endif()

# Locate binaries supplied by the llvm-config
execute_process(COMMAND ${LLVMCONFIG} --bindir OUTPUT_VARIABLE LLVM_TOOLS_BINARY_DIR)
separate_arguments(LLVM_TOOLS_BINARY_DIR NATIVE_COMMAND ${LLVM_TOOLS_BINARY_DIR})
set(CMAKE_CXX_COMPILER "${LLVM_TOOLS_BINARY_DIR}/clang++")
set(CMAKE_C_COMPILER "${LLVM_TOOLS_BINARY_DIR}/clang")

# ====================================================
# Creating a cmake library representing all flags to
# compile and link against LLVM
# ====================================================
add_library(llvm_interface INTERFACE)

execute_process(COMMAND ${LLVMCONFIG} --cppflags OUTPUT_VARIABLE llvm_compile_flags)
separate_arguments(llvm_compile_flags NATIVE_COMMAND ${llvm_compile_flags})
target_compile_options(llvm_interface INTERFACE ${llvm_compile_flags})

execute_process(COMMAND ${LLVMCONFIG} --ldflags OUTPUT_VARIABLE llvm_link_flags)
separate_arguments(llvm_link_flags NATIVE_COMMAND ${llvm_link_flags})
target_link_options(llvm_interface INTERFACE ${llvm_link_flags})

execute_process(COMMAND ${LLVMCONFIG} --system-libs --libs ${LLVM_LIBS} OUTPUT_VARIABLE llvm_link_libs)
separate_arguments(llvm_link_libs NATIVE_COMMAND ${llvm_link_libs})
target_link_libraries(llvm_interface INTERFACE ${llvm_link_libs})

# ====================================================
# Function to set flags for all targets in the project
# ====================================================
function(jlm_target_options target)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
  target_compile_options(${target} PRIVATE -std=c++17 -Wall -Wpedantic -Wextra -Wno-unused-parameter -Werror -Wfatal-errors -gdwarf-4)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${target} PRIVATE -g)
    target_compile_definitions(${target} PRIVATE -DJLM_DEBUG -DJLM_ENABLE_ASSERTS)
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${target} PRIVATE -O3)
  endif()
endfunction()

# ====================================================
# Recursive CMakeLists.txt define all targets
# ====================================================
add_subdirectory(jlm/util)
add_subdirectory(jlm/rvsdg)
add_subdirectory(jlm/llvm)
add_subdirectory(jlm/hls)
add_subdirectory(jlm/tooling)
add_subdirectory(tools)
# add_subdirectory(docs)
add_subdirectory(tests)
