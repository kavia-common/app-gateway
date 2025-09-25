# App2AppProvider CMake configuration
cmake_minimum_required(VERSION 3.16)

project(App2AppProvider)

set(PLUGIN_NAME App2AppProvider)

# Exactly four-file implementation: only two .cpp are compiled
add_library(${PLUGIN_NAME} SHARED
    src/App2AppProvider.cpp
    src/App2AppProviderImplementation.cpp
)

# Include the plugin's own headers and the project root for IApp2AppProvider.h / IAppGateway.h / UtilsLogging.h
target_include_directories(${PLUGIN_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../..     # repository root (contains IApp2AppProvider.h / IAppGateway.h / UtilsLogging.h)
)

# Thunder/WPEFramework linkage
find_package(WPEFramework REQUIRED)
target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        WPEFrameworkCore
        WPEFrameworkCOM
        WPEFrameworkPlugins
)

set_target_properties(${PLUGIN_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# Installation (optional, adapt to your tree)
install(TARGETS ${PLUGIN_NAME} DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/App2AppProvider)
