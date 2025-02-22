cmake_minimum_required(VERSION 3.22.1)
project(rbot)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

if (WIN32)
    message(STATUS "Compiling on Windows")

    # Enable 32-bit compilation
    set(CMAKE_GENERATOR_PLATFORM x86)
    
    # Use /MD (dynamic runtime linking)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD /std:c++11 /W0 /D_CRT_SECURE_NO_WARNINGS /EHsc /bigobj")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")

else()
    message(STATUS "Compiling on Linux")

    # Use 32-bit mode & GoldSrc compatible settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -m32 -w -fvisibility=hidden -fno-exceptions -fno-rtti -Wno-parentheses -pipe -mtune=generic")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
endif()

# Output directory for the plugin library
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Source files
file(GLOB_RECURSE RBOT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../source/*.cpp
)

# Create the shared library
add_library(${PROJECT_NAME} SHARED ${RBOT_SRC})

# Windows-specific export definition for DLLs
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RBOT_DLL_EXPORTS)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
)

# Ensure 32-bit linking
set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)

# Print the effective compiler/linker flags
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
