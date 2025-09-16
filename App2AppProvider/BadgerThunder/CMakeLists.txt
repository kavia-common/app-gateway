cmake_minimum_required(VERSION 3.16)
project(BadgerThunder LANGUAGES CXX DESCRIPTION "Badger Thunder plugin skeleton")

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow downstream to pass WPEFramework dependencies. These are typical names used in Thunder builds.
# If find_package is not available in your environment, adjust or vendor the required cmake config files.
option(USE_WPEFRAMEWORK "Link against WPEFramework libraries if available" ON)

if (USE_WPEFRAMEWORK)
  # These imported targets are commonly exported by Thunder/WPEFramework builds.
  # If your toolchain uses different names, map them in a parent toolchain or override here.
  find_library(WPEFRAMEWORK_CORE NAMES WPEFrameworkCore)
  find_library(WPEFRAMEWORK_PLUGINS NAMES WPEFrameworkPlugins)
endif()

# Sources
set(BADGER_THUNDER_SOURCES
  src/Module.cpp
  src/BadgerThunderPlugin.cpp
)

add_library(WPEFrameworkBadgerThunder SHARED ${BADGER_THUNDER_SOURCES})
target_include_directories(WPEFrameworkBadgerThunder
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link (best-effort)
if (USE_WPEFRAMEWORK AND WPEFRAMEWORK_CORE AND WPEFRAMEWORK_PLUGINS)
  target_link_libraries(WPEFrameworkBadgerThunder PRIVATE ${WPEFRAMEWORK_CORE} ${WPEFRAMEWORK_PLUGINS})
endif()

# Visibility and SONAME
set_target_properties(WPEFrameworkBadgerThunder PROPERTIES
  OUTPUT_NAME "WPEFrameworkBadgerThunder"
  SOVERSION 1
  VERSION 1.0.0
)

# Install rules (adapt paths to your distro layout)
install(TARGETS WPEFrameworkBadgerThunder
  LIBRARY DESTINATION lib/wpeframework/plugins
  ARCHIVE DESTINATION lib/wpeframework/plugins
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

# Install a plugin manifest/config (sample)
install(FILES plugin.json DESTINATION etc/WPEFramework/plugins RENAME BadgerThunder.json)

# Print a helpful message after configuration
message(STATUS "BadgerThunder plugin configured. Adjust USE_WPEFRAMEWORK and dependency names as needed.")
