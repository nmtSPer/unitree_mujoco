cmake_minimum_required(VERSION 3.16)
get_filename_component(ROBOT_NAME "${CMAKE_CURRENT_LIST_DIR}" NAME)
project(${ROBOT_NAME}_controllers)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

set(ROBOT_MAIN "${CMAKE_CURRENT_LIST_DIR}/main.cpp")
if(NOT EXISTS "${ROBOT_MAIN}")
  message(STATUS "No main.cpp found in ${CMAKE_CURRENT_LIST_DIR}")
  return()
endif()

set(UNITREE_SDK2_VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../third_party/unitree_sdk2")
if(EXISTS "${UNITREE_SDK2_VENDOR_DIR}/CMakeLists.txt")
  set(BUILD_EXAMPLES OFF CACHE BOOL "Do not build vendored unitree_sdk2 examples" FORCE)
  add_subdirectory("${UNITREE_SDK2_VENDOR_DIR}" "${CMAKE_BINARY_DIR}/unitree_sdk2" EXCLUDE_FROM_ALL)
else()
  list(APPEND CMAKE_PREFIX_PATH "/opt/unitree_robotics/lib/cmake")
  find_package(unitree_sdk2 REQUIRED)
endif()
find_package(yaml-cpp REQUIRED)

add_executable(main "${ROBOT_MAIN}")
target_link_libraries(main unitree_sdk2 yaml-cpp)
