# CMake Config
cmake_minimum_required(VERSION 3.24)
project(Genode LANGUAGES C CXX)
if (APPLE)
    enable_language(OBJCXX)
endif()

include(CheckIncludeFiles)
include(FetchContent)

# Compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Aliases
set(LIBRARY_NAME "Genode")

# Headers and Sources
file(GLOB_RECURSE SRCS src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.hpp)
file(GLOB_RECURSE HEADERS include/*.hpp)
file(GLOB_RECURSE HEADERS extlibs/headers/*.hpp)
file(GLOB_RECURSE HEADERS extlibs/headers/*.h)

# Library
add_library(${LIBRARY_NAME} STATIC ${SRCS} ${HEADERS})

# Default flags
if (NOT CMAKE_BUILD_TYPE)
    message("-- [Warning] CMAKE_BUILD_TYPE is not set. Falling back to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()
message("-- Build type: " ${CMAKE_BUILD_TYPE})

# Output directory
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
file(MAKE_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set_target_properties(${LIBRARY_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG          ${OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE        ${OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL     ${OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}
)
message("-- Output Directory: " ${OUTPUT_DIR})

if(WIN32)
    # Windows (Visual Studio) Compiler Output Flags
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_target_properties(${LIBRARY_NAME} PROPERTIES
        WIN32_EXECUTABLE     FALSE
        MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY}
    )
elseif(APPLE)
    # Set dependencies libs
    target_link_libraries(${LIBRARY_NAME} PRIVATE "-framework Security")
else()
    # Libraries
    target_link_libraries(${LIBRARY_NAME} PRIVATE stdc++fs)
endif()

# External Libs Sources and Flags
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

# -- SFML
set(SFML_USE_STATIC_STD_LIBS TRUE)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
)
FetchContent_MakeAvailable(SFML)

# -- JSON
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# -- FMT
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        11.0.2
)
FetchContent_MakeAvailable(fmt)

# Application includes
target_include_directories(${LIBRARY_NAME} PRIVATE src)
target_include_directories(${LIBRARY_NAME} PRIVATE include)
target_include_directories(${LIBRARY_NAME} PRIVATE extlibs/headers)

# Link SFML
target_link_libraries(${LIBRARY_NAME} PRIVATE SFML::Audio SFML::Graphics SFML::System SFML::Window)

# Link FreeType
find_package(Freetype)
target_link_libraries(${LIBRARY_NAME} PRIVATE Freetype::Freetype)

# Link JSON
target_link_libraries(${LIBRARY_NAME} PRIVATE nlohmann_json::nlohmann_json)

# Link fmt
target_link_libraries(${LIBRARY_NAME} PRIVATE fmt::fmt)

