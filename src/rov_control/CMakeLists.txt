cmake_minimum_required(VERSION 3.8)
project(rov_control)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(rov_controller src/rov_controller.cpp)
ament_target_dependencies(rov_controller
  rclcpp geometry_msgs std_msgs
)

add_executable(motor_simulator src/motor_simulator.cpp)
ament_target_dependencies(motor_simulator
  rclcpp std_msgs
)

install(TARGETS
  rov_controller
  motor_simulator
  DESTINATION lib/${PROJECT_NAME}
)

install(PROGRAMS
  scripts/dashboard.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
