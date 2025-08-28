cmake_minimum_required(VERSION 3.16)
project(ImGuiRtAudioRtMidiDemo CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------
# 目录结构 (请根据你的实际路径修改)
# -------------------------------
set(IMGUI_DIR    ${CMAKE_SOURCE_DIR}/src)       # imgui*.cpp 和 imgui.h 在 src/
set(BACKENDS_DIR ${CMAKE_SOURCE_DIR}/backends)  # backends 与 src 同层

# -------------------------------
# 源文件
# -------------------------------
add_executable(ImGuiRtAudioRtMidiDemo
  ${IMGUI_DIR}/main.cpp          # 你的主程式
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp    # 可选
  ${BACKENDS_DIR}/imgui_impl_glfw.cpp
  ${BACKENDS_DIR}/imgui_impl_opengl3.cpp
)

# 头文件路径
target_include_directories(ImGuiRtAudioRtMidiDemo PRIVATE
  ${IMGUI_DIR}
  ${BACKENDS_DIR}
)

# -------------------------------
# OpenGL
# -------------------------------
find_package(OpenGL REQUIRED)
target_link_libraries(ImGuiRtAudioRtMidiDemo PRIVATE OpenGL::GL)

# -------------------------------
# GLFW (优先 pkg-config)
# -------------------------------
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(GLFW3 IMPORTED_TARGET glfw3)
endif()

if (TARGET PkgConfig::GLFW3)
  target_link_libraries(ImGuiRtAudioRtMidiDemo PRIVATE PkgConfig::GLFW3)
else()
  message(WARNING "pkg-config 未找到 glfw3，尝试 Homebrew 路径")
  set(HOMEBREW_PREFIX "/opt/homebrew")
  if (NOT EXISTS "${HOMEBREW_PREFIX}/lib/libglfw.dylib")
    set(HOMEBREW_PREFIX "/usr/local")
  endif()
  target_include_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${HOMEBREW_PREFIX}/include)
  target_link_directories(ImGuiRtAudioRtMidiDemo  PRIVATE ${HOMEBREW_PREFIX}/lib)
  target_link_libraries(ImGuiRtAudioRtMidiDemo    PRIVATE glfw)
endif()

# -------------------------------
# RtAudio
# -------------------------------
if (PkgConfig_FOUND)
  pkg_check_modules(RTAUDIO QUIET rtaudio)
endif()

if (RTAUDIO_FOUND)
  message(STATUS "Found RtAudio via pkg-config: ${RTAUDIO_VERSION}")
  target_include_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${RTAUDIO_INCLUDE_DIRS})
  target_link_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${RTAUDIO_LIBRARY_DIRS})
  target_link_libraries(ImGuiRtAudioRtMidiDemo PRIVATE ${RTAUDIO_LIBRARIES})
else()
  message(WARNING "pkg-config 未找到 rtaudio，尝试 Homebrew 路径")
  set(HOMEBREW_PREFIX "/opt/homebrew")
  if (NOT EXISTS "${HOMEBREW_PREFIX}/lib/librtaudio.dylib")
    set(HOMEBREW_PREFIX "/usr/local")
  endif()
  target_include_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${HOMEBREW_PREFIX}/include)
  target_link_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${HOMEBREW_PREFIX}/lib)
  target_link_libraries(ImGuiRtAudioRtMidiDemo PRIVATE rtaudio)
endif()

# -------------------------------
# RtMidi
# -------------------------------
if (PkgConfig_FOUND)
  pkg_check_modules(RTMIDI QUIET rtmidi)
endif()

if (RTMIDI_FOUND)
  message(STATUS "Found RtMidi via pkg-config: ${RTMIDI_VERSION}")
  target_include_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${RTMIDI_INCLUDE_DIRS})
  target_link_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${RTMIDI_LIBRARY_DIRS})
  target_link_libraries(ImGuiRtAudioRtMidiDemo PRIVATE ${RTMIDI_LIBRARIES})
else()
  message(WARNING "pkg-config 未找到 rtmidi，尝试 Homebrew 路径")
  set(HOMEBREW_PREFIX "/opt/homebrew")
  if (NOT EXISTS "${HOMEBREW_PREFIX}/lib/librtmidi.dylib")
    set(HOMEBREW_PREFIX "/usr/local")
  endif()
  target_include_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${HOMEBREW_PREFIX}/include)
  target_link_directories(ImGuiRtAudioRtMidiDemo PRIVATE ${HOMEBREW_PREFIX}/lib)
  target_link_libraries(ImGuiRtAudioRtMidiDemo PRIVATE rtmidi)
endif()

# -------------------------------
# macOS 额外 Frameworks
# -------------------------------
if(APPLE)
  target_compile_definitions(ImGuiRtAudioRtMidiDemo PRIVATE GL_SILENCE_DEPRECATION)
  target_link_libraries(ImGuiRtAudioRtMidiDemo PRIVATE
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework CoreAudio"
    "-framework AudioUnit"
    "-framework AudioToolbox"
    "-framework CoreFoundation"
  )
endif()

# set(CMAKE_VERBOSE_MAKEFILE ON)   # 调试时可打开
