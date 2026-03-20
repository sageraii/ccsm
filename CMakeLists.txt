cmake_minimum_required(VERSION 3.20)
project(ccsm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ftxui CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)

file(GLOB_RECURSE CCSM_SOURCES
    src/core/*.cpp
    src/cli/*.cpp
    src/tui/*.cpp
    src/util/*.cpp
)

add_library(ccsm_lib ${CCSM_SOURCES})
target_include_directories(ccsm_lib PUBLIC src)
target_link_libraries(ccsm_lib PUBLIC
    ftxui::screen ftxui::dom ftxui::component
    CLI11::CLI11
    nlohmann_json::nlohmann_json
)

add_executable(ccsm src/main.cpp)
target_link_libraries(ccsm PRIVATE ccsm_lib)

enable_testing()
add_subdirectory(tests)
