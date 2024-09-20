cmake_minimum_required(VERSION 3.0.0)
set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "1")
set(PROJECT_VERSION_PATCH "0")
project(SBColl VERSION 1.1.0 LANGUAGES CXX)

include(FindPkgConfig)

# Set the default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Find and Check Library
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Specify the source files
set(SOURCE_FILES
    src/sqlite3-log.cpp
    src/main.cpp
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/Serialink/include>
  $<INSTALL_INTERFACE:include>
  ${SQLITE3_INCLUDE_DIRS}
)

# Ensure Serialink is built before the executable
add_dependencies(${PROJECT_NAME} Serialink-lib)

# Link the executable with the library
target_link_libraries(${PROJECT_NAME} PRIVATE Serialink-lib DataFrame-lib)
target_link_libraries(${PROJECT_NAME} PUBLIC -lpthread -lsqlite3)

add_subdirectory(external/Serialink EXCLUDE_FROM_ALL)
set(USE_EXE_FUNC ON)
set(USE_POST_EXE_FUNC ON)
add_definitions(-D__USE_EXE_FUNC)
add_definitions(-D__USE_POST_FUNC)

# Set compiler and linker flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3")
set(CMAKE_USE_RELATIVE_PATHS OFF)
