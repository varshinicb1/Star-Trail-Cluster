cmake_minimum_required(VERSION 3.20)
project(star_trail_simulator C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 from vcpkg
find_package(SDL2 CONFIG REQUIRED)

# Fetch LVGL 8.3.x
include(FetchContent)
FetchContent_Declare(
  lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v8.3.9
)
# PC-portable LVGL config (the firmware's lv_conf.h uses ESP32-only attributes
# that MSVC can't parse). Kept in sync with the firmware's rendering settings:
# same 16-bit colour depth, no byte swap, gradients on.
set(LV_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h CACHE INTERNAL "")
FetchContent_MakeAvailable(lvgl)

set(WIDGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../BenzCluster)

# Widget source files from the firmware
set(WIDGET_SRCS
  ${WIDGET_DIR}/compass.cpp
  ${WIDGET_DIR}/attitude.cpp
  ${WIDGET_DIR}/alttemp.cpp
  ${WIDGET_DIR}/clock.cpp
  ${WIDGET_DIR}/gforce.cpp
  ${WIDGET_DIR}/music.cpp
  ${WIDGET_DIR}/airplane.cpp
)

add_executable(simulator main.cpp ${WIDGET_SRCS})

target_include_directories(simulator PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}  # for Arduino.h, lv_conf.h
  ${WIDGET_DIR}                # for config.h, widget headers
)

target_link_libraries(simulator PRIVATE
  lvgl
  SDL2::SDL2
)

target_compile_definitions(simulator PRIVATE
  LV_CONF_INCLUDE_SIMPLE
  _USE_MATH_DEFINES
)
