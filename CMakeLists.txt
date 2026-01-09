cmake_minimum_required(VERSION 3.10)
project(EpidemicApp CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# 1. 查找外部依赖 (窗口后端和OpenGL)
# ============================================================
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# ============================================================
# 2. 定义 ImGui 的源文件集合
# ============================================================
# 我们显式列出需要编译的文件，不包含多余的后端
set(IMGUI_SOURCES
    src/imgui/imgui.cpp
    src/imgui/imgui_demo.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    # GLFW 后端实现
    src/imgui/backends/imgui_impl_glfw.cpp
    # OpenGL3 渲染后端实现
    src/imgui/backends/imgui_impl_opengl3.cpp

    src/implot/implot.cpp
    src/implot/implot_demo.cpp
    src/implot/implot_items.cpp
)

# ============================================================
# 3. 生成可执行文件
# ============================================================
# 最终程序 = 你的 main.cpp + ImGui 的一堆 cpp
add_executable(EpidemicApp src/main.cpp src/DataModel.cpp ${IMGUI_SOURCES})

# 告诉编译器去哪里找 ImGui 的头文件 (.h)
target_include_directories(EpidemicApp PRIVATE
  src/imgui
  src/imgui/backends
  src/implot
)

# ============================================================
# 4. 链接库
# ============================================================
target_link_libraries(EpidemicApp PRIVATE
    glfw          # 窗口管理库
    OpenGL::GL    # 图形渲染库
    dwmapi        # Windows 系统库(用于窗口边框等杂项)
)
