cmake_minimum_required(VERSION 3.16)
project(FuzzingModule)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force LLVM 19 paths
set(LLVM_DIR "/usr/lib/llvm19/lib/cmake/llvm")
set(CMAKE_PREFIX_PATH "/usr/lib/llvm19/lib/cmake/llvm;${CMAKE_PREFIX_PATH}")

# Find LLVM 19 (accept any 19.x version)
find_package(LLVM 19.1 REQUIRED CONFIG PATHS /usr/lib/llvm19/lib/cmake/llvm NO_DEFAULT_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Get LLVM link flags using llvm-config
execute_process(
    COMMAND /usr/lib/llvm19/bin/llvm-config --ldflags --system-libs --libs all
    OUTPUT_VARIABLE LLVM_LINK_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "LLVM Link Flags: ${LLVM_LINK_FLAGS}")

# Parse the link flags into separate arguments and clean whitespace
string(REPLACE " " ";" LLVM_LINK_FLAGS_LIST ${LLVM_LINK_FLAGS})
# Remove empty strings from the list
list(REMOVE_ITEM LLVM_LINK_FLAGS_LIST "")
# Clean each item
set(LLVM_CLEAN_FLAGS)
foreach(flag ${LLVM_LINK_FLAGS_LIST})
    string(STRIP "${flag}" clean_flag)
    if(NOT clean_flag STREQUAL "")
        list(APPEND LLVM_CLEAN_FLAGS ${clean_flag})
    endif()
endforeach()

# Find nlohmann/json for SARIF format
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Source files
set(SOURCES
    src/fuzzer.cpp
    src/memory_executor.cpp
    src/signal_handler.cpp
    src/sarif_reporter.cpp
    src/bytecode_transformer.cpp
    src/main.cpp
)

# Headers
set(HEADERS
    include/fuzzer.h
    include/memory_executor.h
    include/signal_handler.h
    include/sarif_reporter.h
    include/bytecode_transformer.h
    include/test_result.h
)

# Create executable
add_executable(fuzzing_module ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(fuzzing_module PRIVATE include)

# Link libraries
target_link_libraries(fuzzing_module ${LLVM_CLEAN_FLAGS} nlohmann_json::nlohmann_json pthread)

# Compiler flags
target_compile_options(fuzzing_module PRIVATE -Wall -Wextra -O2)
