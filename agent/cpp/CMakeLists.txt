cmake_minimum_required(VERSION 3.16)
project(stand_go2)

set(UNITREE_SDK2_VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/../../third_party/unitree_sdk2")
if(EXISTS "${UNITREE_SDK2_VENDOR_DIR}/CMakeLists.txt")
  set(BUILD_EXAMPLES OFF CACHE BOOL "Do not build vendored unitree_sdk2 examples" FORCE)
  add_subdirectory("${UNITREE_SDK2_VENDOR_DIR}" "${CMAKE_BINARY_DIR}/unitree_sdk2" EXCLUDE_FROM_ALL)
else()
  list(APPEND CMAKE_PREFIX_PATH "/opt/unitree_robotics/lib/cmake")
  find_package(unitree_sdk2 REQUIRED)
endif()
find_package(yaml-cpp REQUIRED)

add_executable(stand_go2 stand_go2.cpp)
target_link_libraries(stand_go2 unitree_sdk2 yaml-cpp)

SET(CMAKE_BUILD_TYPE Release)
