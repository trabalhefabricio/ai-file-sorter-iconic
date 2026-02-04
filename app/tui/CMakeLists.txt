cmake_minimum_required(VERSION 3.21)

project(AIFileSorterTUI LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AI_FILE_SORTER_TUI_BUILD_TESTS "Build TUI unit tests" OFF)

# Prefer MSVC on Windows if available
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/EHsc)
endif()

# FTXUI - Modern C++ TUI library
include(FetchContent)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0
)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Third-party deps
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(JsonCpp CONFIG QUIET)
if(NOT JsonCpp_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
    add_library(JsonCpp::JsonCpp INTERFACE IMPORTED)
    target_include_directories(JsonCpp::JsonCpp INTERFACE ${JSONCPP_INCLUDE_DIRS})
    target_link_directories(JsonCpp::JsonCpp INTERFACE ${JSONCPP_LIBRARY_DIRS})
    target_link_libraries(JsonCpp::JsonCpp INTERFACE ${JSONCPP_LINK_LIBRARIES})
else()
    if(NOT TARGET JsonCpp::JsonCpp)
        add_library(JsonCpp::JsonCpp INTERFACE IMPORTED)
    endif()
endif()
find_package(fmt REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(Intl REQUIRED)

# Backend library sources (shared with GUI version, Qt-free)
set(BACKEND_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/CategorizationService.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/CategorizationSession.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/ConsistencyPassService.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/DatabaseManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/EmbeddedEnv.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/FileScanner.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/GeminiClient.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/IniConfig.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/LLMClient.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/LLMDownloader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/LocalLLMClient.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/Logger.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/MovableCategorizedFile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/ResultsCoordinator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/UndoManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/Utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/Version.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/WhitelistStore.cpp"
)

# TUI-specific sources
set(TUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main_tui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TuiApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TuiSettings.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TuiLLMSelection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TuiCategorizationProgress.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TuiCategorizationResults.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TuiFileTinder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TuiWhitelistManager.cpp"
)

# Executable
add_executable(aifilesorter-tui ${TUI_SOURCES} ${BACKEND_SOURCES})

target_include_directories(aifilesorter-tui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/llama"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Define TUI build flag to exclude Qt-dependent code paths
target_compile_definitions(aifilesorter-tui PRIVATE AI_FILE_SORTER_TUI_BUILD=1)

if(WIN32)
    set(PRECOMPILED_CPU_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/precompiled/cpu/lib")
    set(PRECOMPILED_CPU_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/precompiled/cpu/bin")
    set(LLAMA_CPU_IMPORT "${PRECOMPILED_CPU_LIB_DIR}/llama.lib")
    set(LLAMA_CPU_DLL "${PRECOMPILED_CPU_BIN_DIR}/llama.dll")
    if(NOT EXISTS "${LLAMA_CPU_IMPORT}")
        message(FATAL_ERROR "Missing ${LLAMA_CPU_IMPORT}. Run app/scripts/build_llama_windows.ps1 cuda=off vcpkgroot=<path> first.")
    endif()
    add_library(llama SHARED IMPORTED)
    set_target_properties(llama PROPERTIES
        IMPORTED_IMPLIB "${LLAMA_CPU_IMPORT}"
        IMPORTED_LOCATION "${LLAMA_CPU_DLL}"
    )
    foreach(libName IN ITEMS ggml ggml-base ggml-cpu)
        set(import_path "${PRECOMPILED_CPU_LIB_DIR}/${libName}.lib")
        set(dll_path "${PRECOMPILED_CPU_BIN_DIR}/${libName}.dll")
        if(NOT EXISTS "${import_path}")
            message(FATAL_ERROR "Missing ${import_path}.")
        endif()
        string(REPLACE "-" "_" imported_target "${libName}")
        add_library(${imported_target} SHARED IMPORTED)
        set_target_properties(${imported_target} PROPERTIES
            IMPORTED_IMPLIB "${import_path}"
            IMPORTED_LOCATION "${dll_path}"
        )
    endforeach()
else()
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../include/external/llama.cpp" "${CMAKE_CURRENT_BINARY_DIR}/llama-build")
    target_link_libraries(aifilesorter-tui PRIVATE llama)
endif()

# Link libraries
target_link_libraries(aifilesorter-tui PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
    CURL::libcurl
    OpenSSL::SSL OpenSSL::Crypto
    SQLite::SQLite3
    JsonCpp::JsonCpp
    fmt::fmt
    spdlog::spdlog
    Intl::Intl
    llama
)

if(WIN32)
    target_link_libraries(aifilesorter-tui PRIVATE ggml ggml_base ggml_cpu)
    target_compile_definitions(aifilesorter-tui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    set(WIN_SYSTEM_LIBS wininet d3d11 dxgi dxguid d3d12 mpr userenv)
    foreach(libName IN LISTS WIN_SYSTEM_LIBS)
        target_link_libraries(aifilesorter-tui PRIVATE ${libName})
    endforeach()
endif()

# Installation
install(TARGETS aifilesorter-tui DESTINATION bin)

# Tests
if(AI_FILE_SORTER_TUI_BUILD_TESTS)
    include(CTest)
    enable_testing()
    
    # Add TUI verification test
    add_test(NAME tui_startup_test
        COMMAND aifilesorter-tui --version
    )
    set_tests_properties(tui_startup_test PROPERTIES
        PASS_REGULAR_EXPRESSION "AI File Sorter TUI"
    )
endif()
