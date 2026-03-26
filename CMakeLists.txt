cmake_minimum_required(VERSION 3.20)
project(Project1_CMAKE VERSION 1.0)

# 設定 C++17 標準
set(CMAKE_CXX_STANDARD 17)

# 強制要求編譯器支援此標準（若不支援則報錯，防止降級到舊版本）
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 禁用編譯器特定擴充（如 gnu++17），僅使用標準 C++（-std=c++17）
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Collect Sources (Only from src/) ───────────────────────────
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/Project1_CMAKE.cpp"
    "${CMAKE_SOURCE_DIR}/Project1_CMAKE.h"
)

# ─── Third-party Libraries (from include/) ──────────────────────
# GLAD
file(GLOB GLAD_SRC
    "${CMAKE_SOURCE_DIR}/include/glad4.6/src/*.c"
    "${CMAKE_SOURCE_DIR}/include/ImNeoSequencer/*.h"
    "${CMAKE_SOURCE_DIR}/include/ImNeoSequencer/*.cpp"
)

# ─── Executable ─────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${GLAD_SRC}
)

# ─── Include Directories ────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/glad4.6/include"
)

# ─── Link .lib Files ────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32.lib
    "${CMAKE_SOURCE_DIR}/lib/glew/glew32.lib"
    "${CMAKE_SOURCE_DIR}/lib/GLFW/glfw3.lib"
    "${CMAKE_SOURCE_DIR}/lib/assimp-vc143-mtd.lib"
)
# ─── Copy .dll Files to Output Directory ────────────────────────
file(GLOB DLL_FILES "${CMAKE_SOURCE_DIR}/dll/*.dll")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL_FILES}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "複製 DLLs 到輸出目錄..."
)

file(GLOB SHADER_FOLDER "${CMAKE_SOURCE_DIR}/ShaderPrograms")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_FOLDER}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/ShaderPrograms
    COMMENT "複製 著色器檔案 到輸出目錄..."
)

file(GLOB SHADER_FOLDER "${CMAKE_SOURCE_DIR}/objModels")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_FOLDER}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/objModels
    COMMENT "複製 3D物件檔案 到輸出目錄..."
)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows")

find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE imguizmo::imguizmo)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)