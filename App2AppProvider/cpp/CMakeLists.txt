cmake_minimum_required(VERSION 3.16)

project(App2AppProvider LANGUAGES CXX)

# Find WPEFramework package
find_package(WPEFramework REQUIRED)

add_library(App2AppProvider SHARED
    App2AppProvider.cpp
)

target_include_directories(App2AppProvider
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Interfaces
)

# Link against WPEFramework libraries
target_link_libraries(App2AppProvider
    PRIVATE
        WPEFrameworkCore
        WPEFrameworkPlugins
        WPEFrameworkCOM
)

target_compile_features(App2AppProvider PRIVATE cxx_std_17)

# Optionally set a soname/output name
set_target_properties(App2AppProvider PROPERTIES
    OUTPUT_NAME "App2AppProvider"
)

# Install the plugin library and manifest
install(TARGETS App2AppProvider
    LIBRARY DESTINATION lib
)

install(FILES plugin.json
    DESTINATION etc/WPEFramework/plugins
)
