cmake_minimum_required(VERSION 3.21)

project(eris VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# raylib (fetched automatically on first configure)
# ---------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# ---------------------------------------------------------------------------
# Game executable
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ERIS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_executable(eris ${ERIS_SOURCES})

if(WIN32)
    target_sources(eris PRIVATE src/application.rc)
endif()

target_include_directories(eris PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(eris PRIVATE raylib)

if(MSVC)
    target_compile_options(eris PRIVATE /W3)
    target_compile_definitions(eris PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Console window in Debug, no console in Release builds.
    set_target_properties(eris PROPERTIES
        LINK_FLAGS_RELEASE        "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_MINSIZEREL     "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:eris>"
    )
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT eris)
endif()

# Copy resources next to the executable so SearchAndSetResourceDir finds them.
add_custom_command(TARGET eris POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:eris>/resources"
)
