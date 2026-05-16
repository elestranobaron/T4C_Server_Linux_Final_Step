cmake_minimum_required(VERSION 3.10)
project(T4CServer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas -Wno-overloaded-virtual -Wno-mismatched-new-delete -Wno-unused-but-set-variable -Wno-ignored-qualifiers -fcheck-new")

# Find MySQL/MariaDB (optional on Linux migrations)
find_library(MYSQL_LIBRARIES NAMES mariadb mysqlclient)
find_path(MYSQL_INCLUDE_DIR NAMES mysql.h mariadb/mysql.h
          HINTS /usr/include /usr/include/mysql /usr/include/mariadb)
if(MYSQL_LIBRARIES AND MYSQL_INCLUDE_DIR)
    message(STATUS "MySQL/MariaDB found: ${MYSQL_LIBRARIES}")
    include_directories(${MYSQL_INCLUDE_DIR})
else()
    message(WARNING "MySQL/MariaDB dev headers/libs not found; continuing without explicit mysql link.")
endif()

# Find SDL3
find_package(SDL3 REQUIRED CONFIG)
if(NOT SDL3_FOUND)
    message(FATAL_ERROR "SDL3 not found. Install libsdl3-dev: sudo apt install libsdl3-dev")
endif()

# Find OpenSSL
find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found. Install libssl-dev: sudo apt install libssl-dev")
endif()

# Find ODBC
find_package(ODBC REQUIRED)
if(NOT ODBC_FOUND)
    message(FATAL_ERROR "ODBC not found. Install unixodbc-dev: sudo apt install unixodbc-dev")
endif()

# Find Threads
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${SDL3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ODBC_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/T4C Server"
    "${CMAKE_SOURCE_DIR}/T4C Server/Core"
    "${CMAKE_SOURCE_DIR}/T4C Server/crypt"
    "${CMAKE_SOURCE_DIR}/T4C Server/NPC_Editor"
    "${CMAKE_SOURCE_DIR}/T4C Server/Skills"
    "${CMAKE_SOURCE_DIR}/T4C Server/SpellEffects"
    "${CMAKE_SOURCE_DIR}/T4C Server/TinyXML"
    "${CMAKE_SOURCE_DIR}/T4C Server/UDP"
    "${CMAKE_SOURCE_DIR}/T4C Server/World Objects"
    ${CMAKE_SOURCE_DIR}/Crypto
    ${CMAKE_SOURCE_DIR}/GameOps
    ${CMAKE_SOURCE_DIR}/NPCs
    "${CMAKE_SOURCE_DIR}/NPC AddOn"
    "${CMAKE_SOURCE_DIR}/NPC Arakas"
    "${CMAKE_SOURCE_DIR}/NPC RavensDust"
    "${CMAKE_SOURCE_DIR}/NPC Remort"
    "${CMAKE_SOURCE_DIR}/NPC Stoneheim"
    "${CMAKE_SOURCE_DIR}/NPC WindHowl"
)

# Source files (.cpp and .CPP — Linux filesystems are case-sensitive; legacy tree uses both.)
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/T4C Server/*.cpp"
    "${CMAKE_SOURCE_DIR}/T4C Server/*.CPP"
    "${CMAKE_SOURCE_DIR}/Crypto/*.cpp"
    "${CMAKE_SOURCE_DIR}/Crypto/*.CPP"
    "${CMAKE_SOURCE_DIR}/GameOps/*.cpp"
    "${CMAKE_SOURCE_DIR}/GameOps/*.CPP"
    "${CMAKE_SOURCE_DIR}/NPCs/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPCs/*.CPP"
    "${CMAKE_SOURCE_DIR}/NPC AddOn/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC AddOn/*.CPP"
    "${CMAKE_SOURCE_DIR}/NPC Arakas/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Arakas/*.CPP"
    "${CMAKE_SOURCE_DIR}/NPC RavensDust/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC RavensDust/*.CPP"
    "${CMAKE_SOURCE_DIR}/NPC Remort/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Remort/*.CPP"
    "${CMAKE_SOURCE_DIR}/NPC Stoneheim/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Stoneheim/*.CPP"
    "${CMAKE_SOURCE_DIR}/NPC WindHowl/*.cpp"
    "${CMAKE_SOURCE_DIR}/NPC WindHowl/*.CPP"
)
# Exclude build and temp directories
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/build/.*")
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/keeprolling/.*")
# System-*.cpp are pulled in via System.cpp; compiling them separately duplicates symbols / breaks Linux.
list(FILTER SOURCES EXCLUDE REGEX ".*/System-(Win32|Linux)\\.[cC][pP][pP]$")
# UDP folder is Win32 client-connection path; Linux server uses POSIX sockets from CommCenter without these sources.
list(FILTER SOURCES EXCLUDE REGEX ".*/UDP/.*\\.[cC][pP][pP]$")
# EXPFLTR.CPP is Windows-only (dbghelp / SEH); Linux uses EXPFLTR_linux.cpp stubs.
list(FILTER SOURCES EXCLUDE REGEX ".*/EXPFLTR\\.CPP$")
# Windows service control (winsvc); not used on Linux (see main.cpp).
list(FILTER SOURCES EXCLUDE REGEX ".*/C_SERVIC\\.CPP$")
# MonsterStatSetup.cpp exists in each regional folder (duplicate MonsterStatSetup symbols on Linux).
# Each unit gets T4C_MONSTER_STAT_SETUP_FUNC_NAME from DLLCreatures.h; dispatch calls them all.
list(FILTER SOURCES EXCLUDE REGEX ".*/MonsterStatSetup\\.cpp$")
set(_mss_paths
    "${CMAKE_SOURCE_DIR}/GameOps/MonsterStatSetup.cpp"
    "${CMAKE_SOURCE_DIR}/NPCs/MonsterStatSetup.cpp"
    "${CMAKE_SOURCE_DIR}/NPC AddOn/MonsterStatSetup.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Arakas/MonsterStatSetup.cpp"
    "${CMAKE_SOURCE_DIR}/NPC RavensDust/MonsterStatSetup.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Remort/MonsterStatSetup.cpp"
    "${CMAKE_SOURCE_DIR}/NPC Stoneheim/MonsterStatSetup.cpp"
    "${CMAKE_SOURCE_DIR}/NPC WindHowl/MonsterStatSetup.cpp"
)
set(_mss_syms GameOps NPCs NPCAddOn Arakas RavensDust Remort Stoneheim WindHowl)
foreach(_mss_path IN LISTS _mss_paths)
    list(APPEND SOURCES "${_mss_path}")
endforeach()
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/T4C Server/MonsterStatSetup_dispatch.cpp")
if(NOT WIN32)
    list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/T4C Server/EXPFLTR_linux.cpp")
endif()

# Create executable
add_executable(T4CServer ${SOURCES})
target_compile_definitions(T4CServer PRIVATE TIXML_USE_STL)
if(NOT WIN32)
    target_compile_definitions(T4CServer PRIVATE NO_DAO_SUPPORT)
    # Per-folder CreateNPC_* + forced include of linux_npc_region/*_rename.h (Windows used separate DLLs).
    set(T4C_NPC_REGION_DIRS GameOps NPCs "NPC AddOn" "NPC Arakas" "NPC RavensDust" "NPC Remort" "NPC Stoneheim" "NPC WindHowl")
    set(T4C_NPC_REGION_SYMS GameOps NPCs NPCAddOn Arakas RavensDust Remort Stoneheim WindHowl)
    set(_ri 0)
    foreach(_dir IN LISTS T4C_NPC_REGION_DIRS)
        list(GET T4C_NPC_REGION_SYMS ${_ri} _sym)
        file(GLOB _reg_src
            "${CMAKE_SOURCE_DIR}/${_dir}/*.cpp"
            "${CMAKE_SOURCE_DIR}/${_dir}/*.CPP")
        set(_rh "${CMAKE_SOURCE_DIR}/T4C Server/linux_npc_region/${_sym}_rename.h")
        set_source_files_properties(${_reg_src} PROPERTIES
            COMPILE_DEFINITIONS "T4C_CREATE_NPC_FUNC_NAME=CreateNPC_${_sym}"
            COMPILE_OPTIONS "-include;${_rh}")
        math(EXPR _ri "${_ri}+1")
    endforeach()
    set(_mss_idx 0)
    foreach(_mss_path IN LISTS _mss_paths)
        list(GET _mss_syms ${_mss_idx} _mss_sym)
        set_property(SOURCE "${_mss_path}" APPEND PROPERTY COMPILE_DEFINITIONS
            "T4C_MONSTER_STAT_SETUP_FUNC_NAME=MonsterStatSetup_${_mss_sym}")
        math(EXPR _mss_idx "${_mss_idx}+1")
    endforeach()
endif()

# Link libraries
target_link_libraries(T4CServer PRIVATE
    SDL3::SDL3
    OpenSSL::SSL
    OpenSSL::Crypto
    ODBC::ODBC
    Threads::Threads
)

if(MYSQL_LIBRARIES)
    target_link_libraries(T4CServer PRIVATE ${MYSQL_LIBRARIES})
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(T4CServer PRIVATE dl)
endif()

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
set(CMAKE_BUILD_TYPE Debug)
