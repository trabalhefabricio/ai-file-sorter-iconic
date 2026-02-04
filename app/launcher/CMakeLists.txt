cmake_minimum_required(VERSION 3.21)

project(AIFileSorterLauncher LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Launcher is a simple console application with no external dependencies
add_executable(aifilesorter-launcher
    main_launcher.cpp
)

# Platform-specific settings
if(WIN32)
    # Link Windows system libraries for process management
    target_link_libraries(aifilesorter-launcher PRIVATE)
    target_compile_definitions(aifilesorter-launcher PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# Installation
install(TARGETS aifilesorter-launcher DESTINATION bin)

# Ensure the launcher is built alongside the main applications
# by including it from the parent CMakeLists.txt
