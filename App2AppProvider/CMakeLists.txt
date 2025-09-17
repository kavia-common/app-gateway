# App2AppProvider CMake configuration
cmake_minimum_required(VERSION 3.16)

project(App2AppProvider)

set(PLUGIN_NAME App2AppProvider)

add_library(${PLUGIN_NAME} SHARED
    src/App2AppProvider.cpp
    src/ProviderRegistry.cpp
    src/CorrelationStore.cpp
    src/AppGatewayClient.cpp
)

target_include_directories(${PLUGIN_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Thunder/WPEFramework linkage - these targets/names align with typical Thunder builds.
# Adjust if your environment uses different target names.
find_package(WPEFramework REQUIRED)
target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        WPEFrameworkCore
        WPEFrameworkProtocols
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
