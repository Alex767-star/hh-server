cmake_minimum_required(VERSION 3.20)
project(hh-server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)
find_package(nlohmann_json REQUIRED)
find_library(SQLITE3_LIB sqlite3 REQUIRED)

add_executable(hh-server
    src/main.cpp
    src/db_handler.cpp
)

target_include_directories(hh-server PRIVATE src)

target_link_libraries(hh-server
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    ${SQLITE3_LIB}
)
