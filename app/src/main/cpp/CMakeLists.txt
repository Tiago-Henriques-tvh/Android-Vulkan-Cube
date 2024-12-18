cmake_minimum_required(VERSION 3.18.1)
project(hellovkjni)

# Include the GameActivity static lib to the project.
find_package(game-activity REQUIRED CONFIG)
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
set(THIRD_PARTY_DIR ../../../../third_party)

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)

add_library(${PROJECT_NAME} SHARED
        vk_main.cpp
        HelloVK.cpp)

# Import the CMakeLists.txt for the glm library
add_subdirectory(${THIRD_PARTY_DIR}/glm ${CMAKE_CURRENT_BINARY_DIR}/glm)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${THIRD_PARTY_DIR}/glm/glm
        ${THIRD_PARTY_DIR}/stb_image)

# add lib dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
        vulkan
        game-activity::game-activity_static
        android
        glm
        log)
