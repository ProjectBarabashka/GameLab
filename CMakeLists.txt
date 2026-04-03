cmake_minimum_required(VERSION 3.16)
project(AETHORIA VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ════════════════════════════════════════════════════════════════
# SFML — ищем автоматически, потом по стандартным путям
# ════════════════════════════════════════════════════════════════
# Если SFML не найден автоматически — укажи путь вручную:
#   cmake .. -DSFML_DIR="C:/путь/к/SFML/lib/cmake/SFML"
# Или раскомментируй строку ниже и поправь путь:
# set(SFML_DIR "C:/Users/PapaZ/Desktop/SFML-2.6.1/lib/cmake/SFML")

if(NOT SFML_DIR)
    # Стандартные места установки
    set(_SFML_HINTS
        "C:/SFML-2.6.1/lib/cmake/SFML"
        "C:/SFML-2.6.0/lib/cmake/SFML"
        "C:/Users/$ENV{USERNAME}/Desktop/SFML-2.6.1/lib/cmake/SFML"
        "C:/Users/PapaZ/Desktop/SFML-2.6.1/lib/cmake/SFML"
        "$ENV{SFML_ROOT}/lib/cmake/SFML"
    )
    find_path(SFML_DIR SFMLConfig.cmake HINTS ${_SFML_HINTS})
endif()

find_package(SFML 2.5 COMPONENTS graphics audio window system REQUIRED)

# ════════════════════════════════════════════════════════════════
# SOURCES
# ════════════════════════════════════════════════════════════════
set(SOURCES
    src/main.cpp
)

set(HEADERS
    src/aethoria_engine.hpp
    src/animation_system.hpp
    src/audio_manager.hpp
    src/entity_system.hpp
    src/json_parser.hpp
    src/prefab_system.hpp
    src/scene_system.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ════════════════════════════════════════════════════════════════
# LINK
# ════════════════════════════════════════════════════════════════
target_link_libraries(${PROJECT_NAME} PRIVATE
    sfml-graphics
    sfml-audio
    sfml-window
    sfml-system
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

# ════════════════════════════════════════════════════════════════
# COMPILER FLAGS
# ════════════════════════════════════════════════════════════════
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /O2 /utf-8)
    # Убираем надоедливые предупреждения MSVC
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)
endif()

# ════════════════════════════════════════════════════════════════
# POST-BUILD: копируем assets и DLL
# ════════════════════════════════════════════════════════════════
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copying assets → build dir..."
)

if(WIN32)
    # Копируем SFML DLL в папку с .exe
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-audio>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying SFML DLLs..."
    )
    # Копируем openal32.dll если есть (нужен для звука)
    find_file(OPENAL_DLL
        NAMES openal32.dll
        HINTS "${SFML_DIR}/../../../bin" "$ENV{SFML_ROOT}/bin"
    )
    if(OPENAL_DLL)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OPENAL_DLL}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    endif()
endif()

# ════════════════════════════════════════════════════════════════
# INFO
# ════════════════════════════════════════════════════════════════
message(STATUS "")
message(STATUS "══════════════════════════════════════")
message(STATUS "  AETHORIA: Eternal Realms v${PROJECT_VERSION}")
message(STATUS "══════════════════════════════════════")
message(STATUS "  SFML_DIR    : ${SFML_DIR}")
message(STATUS "  Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "══════════════════════════════════════")
message(STATUS "")
