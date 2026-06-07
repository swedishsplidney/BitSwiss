cmake_minimum_required(VERSION 3.14)
project(BitSwiss CXX)

# force vcpkg to look for static curl on Windows
if(WIN32)
    set(CURL_USE_STATIC_LIBS ON)
endif()

if(MSVC)
    # force msvc to link c/c++ runtimes
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

# fetch dependencies automatically
include(FetchContent)

# fetch GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(glfw)

# fetch dear imgui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# core source files that are identical between OS
set(SOURCES
        src/main.cpp
        src/StorageManager.hpp
        src/PackageManager.hpp
        src/ThemeManager.hpp
        src/StorageWriter.hpp
        src/StorageWriter.cpp
        src/DownloadManager.hpp
        src/DownloadManager.cpp
)

# conditional source mappings based on the OS
if(WIN32)
    list(APPEND SOURCES src/StorageManager_Windows.cpp)
    message(STATUS "configuring build for Windows...")
elseif(APPLE)
    list(APPEND SOURCES src/StorageManager_Mac.cpp)
    message(STATUS "configuring build for macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND SOURCES src/StorageManager_Linux.cpp)
    message(STATUS "configuring build for Linux")
endif()

# force static linking for windows
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

# add imgui source files manually
target_sources(${PROJECT_NAME} PRIVATE
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        # include glfw and opengl3 backends
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# tell the compiler where the imgui headers are
target_include_directories(${PROJECT_NAME} PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)

# link opengl and system libraries
find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        OpenGL::GL
)

# linux specific linkings (specifically POSIX)
if(NOT WIN32 AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 pthread dl)
endif()