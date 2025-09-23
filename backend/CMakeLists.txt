cmake_minimum_required(VERSION 3.5)
project(fulmen_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories(src)

add_library(fulmen_backend SHARED
  src/container.cpp
)

set_target_properties(fulmen_backend PROPERTIES SUFFIX ".so")

target_include_directories(fulmen_backend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src}
)

if(NOT WIN32)
  target_compile_options(fulmen_backend PRIVATE
    -fvisibility=hidden
  )
endif()

