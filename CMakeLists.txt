cmake_minimum_required(VERSION 3.16)
project(FuzzingModule)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific LLVM paths
if(APPLE)
    # macOS with Homebrew
    if(EXISTS "/opt/homebrew/opt/llvm@19")
        set(LLVM_DIR "/opt/homebrew/opt/llvm@19/lib/cmake/llvm")
    elseif(EXISTS "/usr/local/opt/llvm@19")
        set(LLVM_DIR "/usr/local/opt/llvm@19/lib/cmake/llvm")
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux paths
    if(EXISTS "/usr/lib/llvm19/lib/cmake/llvm")
        set(LLVM_DIR "/usr/lib/llvm19/lib/cmake/llvm")
    elseif(EXISTS "/usr/lib/llvm-19/lib/cmake/llvm")
        set(LLVM_DIR "/usr/lib/llvm-19/lib/cmake/llvm")
    endif()
endif()

# Set CMAKE_PREFIX_PATH if LLVM_DIR is found
if(LLVM_DIR)
    set(CMAKE_PREFIX_PATH "${LLVM_DIR};${CMAKE_PREFIX_PATH}")
    message(STATUS "Using LLVM_DIR: ${LLVM_DIR}")
endif()

# Find LLVM 19 (accept any 19.x version)
find_package(LLVM 19.1 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})

# Handle LLVM definitions safely
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
if(LLVM_DEFINITIONS)
    # Clean any newlines or problematic characters
    string(REPLACE "\n" " " LLVM_DEFINITIONS_CLEAN "${LLVM_DEFINITIONS}")
    string(REPLACE "\r" " " LLVM_DEFINITIONS_CLEAN "${LLVM_DEFINITIONS_CLEAN}")
    
    if(NOT "${LLVM_DEFINITIONS_CLEAN}" STREQUAL "")
        separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS_CLEAN})
        add_definitions(${LLVM_DEFINITIONS_LIST})
        message(STATUS "Applied LLVM definitions: ${LLVM_DEFINITIONS_LIST}")
    else()
        message(STATUS "LLVM_DEFINITIONS is empty after cleaning")
    endif()
else()
    message(STATUS "LLVM_DEFINITIONS is not set")
endif()

# Get LLVM link flags using llvm-config
set(LLVM_CONFIG_PATHS
    "llvm-config"
    "/usr/lib/llvm19/bin/llvm-config"
    "/usr/lib/llvm-19/bin/llvm-config"
    "/opt/homebrew/opt/llvm@19/bin/llvm-config"
    "/usr/local/opt/llvm@19/bin/llvm-config"
)

set(LLVM_CONFIG_CMD "")
foreach(llvm_config_path ${LLVM_CONFIG_PATHS})
    execute_process(
        COMMAND which ${llvm_config_path}
        RESULT_VARIABLE LLVM_CONFIG_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(LLVM_CONFIG_RESULT EQUAL 0)
        set(LLVM_CONFIG_CMD ${llvm_config_path})
        message(STATUS "Found llvm-config at: ${LLVM_CONFIG_CMD}")
        break()
    endif()
endforeach()

if(LLVM_CONFIG_CMD)
    execute_process(
        COMMAND ${LLVM_CONFIG_CMD} --ldflags --system-libs --libs all
        OUTPUT_VARIABLE LLVM_LINK_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE LLVM_CONFIG_SUCCESS
    )
    message(STATUS "LLVM Link Flags: ${LLVM_LINK_FLAGS}")
    
    # Parse the link flags into separate arguments and clean whitespace
    if(LLVM_CONFIG_SUCCESS EQUAL 0 AND LLVM_LINK_FLAGS AND NOT "${LLVM_LINK_FLAGS}" STREQUAL "")
        string(REPLACE " " ";" LLVM_LINK_FLAGS_LIST ${LLVM_LINK_FLAGS})
        # Remove empty strings from the list
        list(REMOVE_ITEM LLVM_LINK_FLAGS_LIST "")
        
        # Separate linker options from libraries
        set(LLVM_CLEAN_FLAGS)
        set(LLVM_LINKER_OPTIONS)
        
        foreach(flag ${LLVM_LINK_FLAGS_LIST})
            string(STRIP "${flag}" clean_flag)
            if(NOT clean_flag STREQUAL "")
                # Check if it's a linker option (starts with -Wl, or -L or other non-library flags)
                if(clean_flag MATCHES "^-Wl," OR clean_flag MATCHES "^-L" OR clean_flag MATCHES "^-framework")
                    list(APPEND LLVM_LINKER_OPTIONS ${clean_flag})
                else()
                    list(APPEND LLVM_CLEAN_FLAGS ${clean_flag})
                endif()
            endif()
        endforeach()
        message(STATUS "Using LLVM link flags from llvm-config")
    else()
        message(WARNING "llvm-config failed or returned empty flags, using LLVM libraries directly")
        set(LLVM_CLEAN_FLAGS ${LLVM_LIBRARIES})
        set(LLVM_LINKER_OPTIONS)
    endif()
else()
    message(WARNING "llvm-config not found, using LLVM libraries directly")
    set(LLVM_CLEAN_FLAGS ${LLVM_LIBRARIES})
    set(LLVM_LINKER_OPTIONS)
endif()

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
    src/cli_parser.cpp
    src/fuzzing_application.cpp
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
    include/cli_parser.h
    include/fuzzing_application.h
)

# Create executable
add_executable(fuzzing_module ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(fuzzing_module PRIVATE include)

# Link libraries
target_link_libraries(fuzzing_module ${LLVM_CLEAN_FLAGS} nlohmann_json::nlohmann_json pthread)

# Add linker options separately for macOS compatibility
if(LLVM_LINKER_OPTIONS)
    target_link_options(fuzzing_module PRIVATE ${LLVM_LINKER_OPTIONS})
endif()

# Compiler flags
target_compile_options(fuzzing_module PRIVATE -Wall -Wextra -O2)
