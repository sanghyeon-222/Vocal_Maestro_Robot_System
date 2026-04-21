cmake_minimum_required(VERSION 3.8)
project(turtlebot_map_viewer)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_default_plugins REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(nav_msgs REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS Widgets Core Sql)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    dbclient.cpp
    dbclient.h
)

add_executable(turtlebot_map_viewer ${PROJECT_SOURCES})

ament_target_dependencies(turtlebot_map_viewer
  rclcpp
  rviz_common
  rviz_default_plugins
  tf2_ros
  nav_msgs
)

target_link_libraries(turtlebot_map_viewer
  Qt5::Widgets
  Qt5::Core
  Qt5::Sql
)

install(TARGETS turtlebot_map_viewer
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
