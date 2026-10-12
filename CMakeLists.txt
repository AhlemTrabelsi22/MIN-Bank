
cmake_minimum_required(VERSION 3.20)

project(MINBankAPI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)

add_executable(min-bank-api
    api_main.cpp
    Database.cpp
)

target_include_directories(min-bank-api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(min-bank-api PRIVATE
    Drogon::Drogon
    PostgreSQL::PostgreSQL
)