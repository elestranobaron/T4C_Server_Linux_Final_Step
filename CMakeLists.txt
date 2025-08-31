cmake_minimum_required(VERSION 3.10)
project(T4CServer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Find dependencies
find_package(SDL3 REQUIRED CONFIG)
if(NOT SDL3_FOUND)
    message(FATAL_ERROR "SDL3 not found. Install libsdl3-dev: sudo apt install libsdl3-dev")
endif()

find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found. Install libssl-dev: sudo apt install libssl-dev")
endif()

find_package(ODBC REQUIRED)
if(NOT ODBC_FOUND)
    message(FATAL_ERROR "ODBC not found. Install unixodbc-dev: sudo apt install unixodbc-dev")
endif()

find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${SDL3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ODBC_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/T4C Server
    ${CMAKE_SOURCE_DIR}/T4C Server/Core
    ${CMAKE_SOURCE_DIR}/T4C Server/crypt
    ${CMAKE_SOURCE_DIR}/T4C Server/NPC_Editor
    ${CMAKE_SOURCE_DIR}/T4C Server/Skills
    ${CMAKE_SOURCE_DIR}/T4C Server/SpellEffects
    ${CMAKE_SOURCE_DIR}/T4C Server/TinyXML
    ${CMAKE_SOURCE_DIR}/T4C Server/UDP
    ${CMAKE_SOURCE_DIR}/T4C Server/World Objects
    ${CMAKE_SOURCE_DIR}/Crypto
    ${CMAKE_SOURCE_DIR}/GameOps
    ${CMAKE_SOURCE_DIR}/NPCs
    ${CMAKE_SOURCE_DIR}/NPC AddOn
    ${CMAKE_SOURCE_DIR}/NPC Arakas
    ${CMAKE_SOURCE_DIR}/NPC RavensDust
    ${CMAKE_SOURCE_DIR}/NPC Remort
    ${CMAKE_SOURCE_DIR}/NPC Stoneheim
    ${CMAKE_SOURCE_DIR}/NPC WindHowl
)

# Source files
file(GLOB_RECURSIVE SOURCES
    "${CMAKE_SOURCE_DIR}/T4C Server/*.cpp"
    "${CMAKE_SOURCE_DIR}/Crypto/*.cpp"
    "${CMAKE_SOURCE_DIR}/GameOps/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPCs/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC AddOn/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Arakas/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC RavensDust/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Remort/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Stoneheim/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC WindHowl/*.cpp"
)

# Exclude build and temp directories
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/build/.*")
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/keeprolling/.*")

# Create executable
add_executable(T4CServer ${SOURCES})

# Link libraries
target_link_libraries(T4CServer PRIVATE
    SDL3::SDL3
    OpenSSL::SSL
    OpenSSL::Crypto
    ODBC::ODBC
    Threads::Threads
)

# Installation
install(TARGETS T4CServer DESTINATION bin)

# Custom target for debugging
add_custom_target(run
    COMMAND T4CServer
    DEPENDS T4CServer
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running T4CServer"
)

# Print build info
message(STATUS "Project directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "SDL3 include dirs: ${SDL3_INCLUDE_DIRS}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
