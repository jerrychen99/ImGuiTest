cmake_minimum_required(VERSION 3.16)
project(ImGuiTest CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 你的目录结构
set(IMGUI_DIR    ${CMAKE_SOURCE_DIR}/src)       # imgui*.cpp 和 imgui.h 在 src/
set(BACKENDS_DIR ${CMAKE_SOURCE_DIR}/backends)  # backends 与 src 同层

# 源文件（不包含 demo，可按需加上 imgui_demo.cpp）
add_executable(ImGuiTest
  ${IMGUI_DIR}/main.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${BACKENDS_DIR}/imgui_impl_glfw.cpp
  ${BACKENDS_DIR}/imgui_impl_opengl3.cpp
)

# 头文件搜索路径（让 #include "imgui.h" / "imgui_impl_*.h" 能找到）
target_include_directories(ImGuiTest PRIVATE
  ${IMGUI_DIR}
  ${BACKENDS_DIR}
)

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(ImGuiTest PRIVATE OpenGL::GL)

# 用 pkg-config 查找并链接 glfw（Homebrew 安装）
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(GLFW3 IMPORTED_TARGET glfw3)
endif()

if (TARGET PkgConfig::GLFW3)
  # 使用 pkg-config 提供的导入目标（会自动携带包含路径与库）
  target_link_libraries(ImGuiTest PRIVATE PkgConfig::GLFW3)
else()
  # 兜底：直接指向常见的 Homebrew 前缀（Apple Silicon: /opt/homebrew；Intel: /usr/local）
  set(HOMEBREW_PREFIX "/opt/homebrew")
  if (NOT EXISTS "${HOMEBREW_PREFIX}/lib/libglfw.dylib")
    set(HOMEBREW_PREFIX "/usr/local")
  endif()
  target_include_directories(ImGuiTest PRIVATE ${HOMEBREW_PREFIX}/include)
  target_link_directories(ImGuiTest  PRIVATE ${HOMEBREW_PREFIX}/lib)
  target_link_libraries(ImGuiTest    PRIVATE glfw)
endif()

# macOS 需要的系统框架（GLFW 依赖）
if(APPLE)
  target_compile_definitions(ImGuiTest PRIVATE GL_SILENCE_DEPRECATION)
  target_link_libraries(ImGuiTest PRIVATE
    "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()
