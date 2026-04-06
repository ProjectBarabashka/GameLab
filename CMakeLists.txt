cmake_minimum_required(VERSION 3.16)
project(AETHORIA VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Пути к SFML
set(SFML_ROOT "C:/SFML/SFML-2.6.1")
set(SFML_DIR "${SFML_ROOT}/lib/cmake/SFML")

find_package(SFML 2.6 COMPONENTS graphics audio window system REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-audio sfml-window sfml-system)
target_include_directories(${PROJECT_NAME} PRIVATE src)

# --- АВТОМАТИЗАЦИЯ КОПИРОВАНИЯ ---

# 1. Копируем ассеты
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets")

# 2. Копируем все DLL из папки SFML/bin
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${SFML_ROOT}/bin/sfml-graphics-d-2.dll"
    "${SFML_ROOT}/bin/sfml-window-d-2.dll"
    "${SFML_ROOT}/bin/sfml-system-d-2.dll"
    "${SFML_ROOT}/bin/sfml-audio-d-2.dll"
    "${SFML_ROOT}/bin/openal32.dll"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
