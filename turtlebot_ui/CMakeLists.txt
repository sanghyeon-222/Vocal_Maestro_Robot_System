cmake_minimum_required(VERSION 3.5)

project(turtlebot_ui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
    main.cpp
    widget.cpp
    widget.h
    widget.ui
    dbclient.cpp
    dbclient.h
)

add_executable(turtlebot_ui ${PROJECT_SOURCES})

target_link_libraries(turtlebot_ui PRIVATE
    Qt5::Widgets
    Qt5::Sql
)
