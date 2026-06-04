cmake_minimum_required(VERSION 3.16)
project(unitree_mujoco)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  message("Debug mode")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -O0 -fPIC")
else()
  message("Release mode")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -DNDEBUG -fPIC")
endif()

set(UNITREE_SDK2_VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/unitree_sdk2")
if(EXISTS "${UNITREE_SDK2_VENDOR_DIR}/CMakeLists.txt")
  set(BUILD_EXAMPLES OFF CACHE BOOL "Do not build vendored unitree_sdk2 examples" FORCE)
  add_subdirectory("${UNITREE_SDK2_VENDOR_DIR}" "${CMAKE_BINARY_DIR}/unitree_sdk2" EXCLUDE_FROM_ALL)
else()
  list(APPEND CMAKE_PREFIX_PATH "/opt/unitree_robotics/lib/cmake")
  find_package(unitree_sdk2 REQUIRED)
endif()
find_package(Boost REQUIRED COMPONENTS program_options)


include_directories(
  /usr/local/include/iceoryx/v2.0.2
  mujoco/include
  mujoco/simulate
  src/lodepng
)
link_directories(mujoco/lib)

file(GLOB SIM_SRC
  src/joystick/joystick.cc

  # exclude mujoco/simulate/main.cc
  mujoco/simulate/glfw_*.cc
  mujoco/simulate/platform_*.cc
  mujoco/simulate/simulate.cc
  src/lodepng/lodepng.cpp
)

link_libraries(
  pthread
  mujoco
  glfw
  yaml-cpp
  unitree_sdk2
  boost_program_options
  fmt
)

add_executable(unitree_mujoco ${SIM_SRC} src/main.cc)

add_executable(jstest src/joystick/jstest.cc src/joystick/joystick.cc)
