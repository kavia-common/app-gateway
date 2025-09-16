cmake_minimum_required(VERSION 3.16)
project(Badger LANGUAGES CXX DESCRIPTION "Badger Thunder plugin")

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(USE_WPEFRAMEWORK "Link against WPEFramework libraries if available" ON)

if (USE_WPEFRAMEWORK)
  find_library(WPEFRAMEWORK_CORE NAMES WPEFrameworkCore)
  find_library(WPEFRAMEWORK_PLUGINS NAMES WPEFrameworkPlugins)
endif()

# Sources
set(BADGER_SOURCES
  src/Module.cpp
  src/BadgerPlugin.cpp
  src/Registry.cpp
  src/PermissionService.cpp
)

add_library(WPEFrameworkBadger SHARED ${BADGER_SOURCES})
target_include_directories(WPEFrameworkBadger
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if (USE_WPEFRAMEWORK AND WPEFRAMEWORK_CORE AND WPEFRAMEWORK_PLUGINS)
  target_link_libraries(WPEFrameworkBadger PRIVATE ${WPEFRAMEWORK_CORE} ${WPEFRAMEWORK_PLUGINS})
endif()

set_target_properties(WPEFrameworkBadger PROPERTIES
  OUTPUT_NAME "WPEFrameworkBadger"
  SOVERSION 1
  VERSION 1.0.0
)

install(TARGETS WPEFrameworkBadger
  LIBRARY DESTINATION lib/wpeframework/plugins
  ARCHIVE DESTINATION lib/wpeframework/plugins
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)
install(FILES plugin.json DESTINATION etc/WPEFramework/plugins RENAME Badger.json)

message(STATUS "Badger plugin configured. USE_WPEFRAMEWORK=${USE_WPEFRAMEWORK}")
