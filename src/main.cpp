#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifdef __APPLE__
  #include <OpenGL/gl3.h>
#else
  #include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>


#include <cstdio>

int main() {
    if (!glfwInit()) return 1;

    // OpenGL 3.2 Core（macOS 需要 forward-compat）
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(960, 600, "ImGui + GLFW + OpenGL3", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    bool show_demo = true;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);
        //ImGui::SetNextWindowViewport(vp->ID);
        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize   |
            ImGuiWindowFlags_NoMove     |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0)); // 大窗无内边距
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(0,0)); // 控件之间无间距

        ImGui::Begin("Window", nullptr,
                    host_flags);

        double avail = ImGui::GetContentRegionAvail().x;
        double base = avail / 3;
        double rem = avail - base * 3;

        double w0 = base + (rem > 0);
        double w1 = base + (rem > 1);
        double w2 = base;

        // 左
        ImGui::BeginChild("LeftPane", ImVec2(w0, 0),
                        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened);
        ImGui::Text("Left");
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);

        // 中
        ImGui::BeginChild("MidPane", ImVec2(w1, 0),
                        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened);
        ImGui::Text("Middle");
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);

        ImGui::BeginChild("RightPane", ImVec2(w2, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened);
        // 右（宽度 0 表示吃掉剩余）
        float availy = ImGui::GetContentRegionAvail().y;
        float h1 = availy / 2;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::BeginChild("RightUpperPane", ImVec2(0, h1),
                        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding |ImGuiChildFlags_NavFlattened);
        ImGui::Text("RightUpper");
        ImGui::EndChild();

        ImGui::BeginChild("RightDownPane", ImVec2(0, 0),
                        ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened);
        ImGui::Text("RightDown");
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::End();
        ImGui::PopStyleVar(2);

        //if (show_demo) ImGui::ShowDemoWindow(&show_demo);

        ImGui::Render();
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
