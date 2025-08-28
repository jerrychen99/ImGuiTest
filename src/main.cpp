#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#ifdef __APPLE__
  #include <OpenGL/gl3.h>
#else
  #include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>

#include <RtAudio.h>
#include <RtMidi.h>

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <mutex>
#include <atomic>

// -------------------------------
// 全局状态
// -------------------------------
std::vector<std::string> audioNames;
std::vector<unsigned int> audioIds;     // 与上面名称一一对应
int selectedAudioIdx = -1;

std::vector<std::string> midiInNames;
int selectedMidiIdx = -1;

std::vector<float> audioBuffer(1024, 0.0f);
std::mutex audioMutex;

std::deque<std::string> midiLog;
std::mutex midiMutex;
std::string lastMidiMessage = "No message";

RtAudio gAudio;        // 保持存活整个程序周期
RtMidiIn gMidiIn;      // 同上
RtMidiOut gMidiOut;
std::atomic<bool> gAudioRunning{false};

// -------------------------------
// RtAudio 回调：把输入的单声道数据拷进 audioBuffer 前部
// -------------------------------
int audioCallback(void* outputBuffer, void* inputBuffer,
                  unsigned int nFrames, double /*streamTime*/,
                  RtAudioStreamStatus status, void* /*userData*/) {
    if (status) std::cerr << "RtAudio stream under/overflow!" << std::endl;
    if (!inputBuffer) return 0;

    float* in = static_cast<float*>(inputBuffer);
    std::lock_guard<std::mutex> lock(audioMutex);
    // 简单显示：如果 nFrames 超过缓冲大小，只取前面一段
    unsigned int copyN = std::min<unsigned int>(nFrames, (unsigned int)audioBuffer.size());
    for (unsigned int i = 0; i < copyN; i++) audioBuffer[i] = in[i];
    // 其余保持原值
    return 0;
}

// -------------------------------
// MIDI 回调：更新 last + push 到日志（最多 10 条）
// -------------------------------
void midiCallback(double /*dt*/, std::vector<unsigned char>* msg, void* /*user*/) {
    if (!msg || msg->empty()) return;
    std::string line = "MIDI:";
    for (auto b : *msg) { line.push_back(' '); line += std::to_string((int)b); }

    {
        std::lock_guard<std::mutex> lk(midiMutex);
        midiLog.push_front(line);
        if (midiLog.size() > 10) midiLog.pop_back();
    }
    lastMidiMessage = line;
}

// -------------------------------
// 打开指定输入设备的音频流（单声道 44100 / float）
// -------------------------------
bool OpenAudioInputByDeviceId(unsigned int deviceId) {
    // 若已有流在跑，先停掉
    try {
        if (gAudio.isStreamOpen()) {
            if (gAudio.isStreamRunning()) gAudio.stopStream();
            gAudio.closeStream();
        }
    } catch (RtAudioErrorType& e) {
        std::cerr << "Stop/Close previous stream failed: " << e << std::endl;
    }

    RtAudio::StreamParameters inParams;
    inParams.deviceId = deviceId;
    inParams.nChannels = 1;
    inParams.firstChannel = 0;

    unsigned int bufferFrames = 1024;
    try {
        gAudio.openStream(nullptr, &inParams, RTAUDIO_FLOAT32, 44100, &bufferFrames, &audioCallback);
        gAudio.startStream();
        gAudioRunning = true;
        return true;
    } catch (RtAudioErrorType& e) {
        std::cerr << "Open/start audio stream failed: " << e << std::endl;
        gAudioRunning = false;
        return false;
    }
}

// -------------------------------
// 关闭音频流
// -------------------------------
void CloseAudio() {
    try {
        if (gAudio.isStreamOpen()) {
            if (gAudio.isStreamRunning()) gAudio.stopStream();
            gAudio.closeStream();
        }
    } catch (RtAudioErrorType& e) {
        std::cerr << "Close audio stream failed: " << e << std::endl;
    }
    gAudioRunning = false;
}

// -------------------------------
// 打开指定 MIDI 输入端口
// -------------------------------
bool OpenMidiInPort(unsigned int port) {
    try {
        if (gMidiIn.isPortOpen()) gMidiIn.closePort();
        gMidiIn.openPort(port);
        gMidiIn.setCallback(&midiCallback);
        // 可选：过滤系统消息
        gMidiIn.ignoreTypes(false, false, false);
        return true;
    } catch (RtMidiError& e) {
        std::cerr << "Open MIDI port failed: " << e.getMessage() << std::endl;
        return false;
    }
}

int main() {
    // -------------------------------
    // GLFW / OpenGL / ImGui
    // -------------------------------
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(960, 600, "ImGui + RtAudio + RtMidi (Split + Selectable)", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // 加载一个支持中文的字体
    io.Fonts->AddFontFromFileTTF(
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf", // 或者 NotoSansCJK-Regular.ttc 的路径
        18.0f,
        nullptr,
        io.Fonts->GetGlyphRangesChineseFull()
    );

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // -------------------------------
    // 枚举音频设备
    // -------------------------------
    try {
        auto ids = gAudio.getDeviceIds();                 // 新 API（如果你的 RtAudio 较旧，可改用 0..getDeviceCount()-1 轮询）
        for (auto id : ids) {
            try {
                auto info = gAudio.getDeviceInfo(id);
                if (!info.name.empty() && info.inputChannels > 0) {
                    audioIds.push_back(id);
                    audioNames.push_back(info.name);
                }
            } catch (...) {
                // 某些 deviceId 可能 probe 失败，忽略
            }
        }
    } catch (...) {
        std::cerr << "Enumerate audio devices failed." << std::endl;
    }

    // 默认选择“默认输入设备”对应的那一项
    if (!audioIds.empty()) {
        unsigned int defIn = gAudio.getDefaultInputDevice();
        selectedAudioIdx = 0;
        for (int i = 0; i < (int)audioIds.size(); ++i) {
            if (audioIds[i] == defIn) { selectedAudioIdx = i; break; }
        }
        OpenAudioInputByDeviceId(audioIds[selectedAudioIdx]);
    }

    try {
        if (gMidiOut.getPortCount() > 0) {
            gMidiOut.openPort(0); // 打开第一个输出端口
        } else {
            std::cerr << "No MIDI OUT ports available.\n";
        }
    } catch (RtMidiError& e) {
        std::cerr << "Open MIDI out failed: " << e.getMessage() << std::endl;
    }


    // -------------------------------
    // 枚举 MIDI 输入端口
    // -------------------------------
    try {
        unsigned int n = gMidiIn.getPortCount();
        for (unsigned int i = 0; i < n; i++) midiInNames.push_back(gMidiIn.getPortName(i));
        if (!midiInNames.empty()) {
            selectedMidiIdx = 0;
            OpenMidiInPort(0);
        }
    } catch (RtMidiError& e) {
        std::cerr << "List MIDI ports failed: " << e.getMessage() << std::endl;
    }

    // -------------------------------
    // UI 主循环
    // -------------------------------
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);

        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize   |
            ImGuiWindowFlags_NoMove     |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(0,0));

        ImGui::Begin("MainHost", nullptr, host_flags);

        double avail = ImGui::GetContentRegionAvail().x;
        double base = avail / 3;
        double rem = avail - base * 3;
        double w0 = base + (rem > 0);
        double w1 = base + (rem > 1);
        double w2 = base;

        // ---------------- 左栏：音频设备 ----------------
        ImGui::BeginChild("LeftPane", ImVec2(w0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        ImGui::Text("Audio Input Device");
        if (audioNames.empty()) {
            ImGui::TextDisabled("No audio input devices");
        } else {
            const char* preview = (selectedAudioIdx >= 0) ? audioNames[selectedAudioIdx].c_str() : "Select...";
            if (ImGui::BeginCombo("##AudioDevice", preview)) {
                for (int i = 0; i < (int)audioNames.size(); ++i) {
                    bool selected = (i == selectedAudioIdx);
                    if (ImGui::Selectable(audioNames[i].c_str(), selected)) {
                        selectedAudioIdx = i;
                        OpenAudioInputByDeviceId(audioIds[selectedAudioIdx]);
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::Separator();
            ImGui::Text("Stream: %s", gAudioRunning.load() ? "Running" : "Stopped");
            if (ImGui::Button(gAudioRunning.load() ? "Stop" : "Start")) {
                if (gAudioRunning.load()) CloseAudio();
                else if (selectedAudioIdx >= 0) OpenAudioInputByDeviceId(audioIds[selectedAudioIdx]);
            }
        }
        ImGui::Separator();
        // 测试按钮：发送一条 Note On / Note Off
        if (ImGui::Button("Send Test MIDI")) {
            if (gMidiOut.getPortCount() > 0) {
                std::vector<unsigned char> msg;
                msg.push_back(0x90); // Note On ch1
                msg.push_back(60);   // Middle C
                msg.push_back(100);  // Velocity
                try { gMidiOut.sendMessage(&msg); } catch(...) {}

                msg[2] = 0;          // Note Off (velocity = 0)
                try { gMidiOut.sendMessage(&msg); } catch(...) {}
            }
        }
        ImGui::Text("Last MIDI: %s", lastMidiMessage.c_str());
        
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);

        // ---------------- 中栏：MIDI 输入 ----------------
        ImGui::BeginChild("MidPane", ImVec2(w1, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        ImGui::Text("MIDI Input Port");
        if (midiInNames.empty()) {
            ImGui::TextDisabled("No MIDI inputs");
        } else {
            const char* preview = (selectedMidiIdx >= 0) ? midiInNames[selectedMidiIdx].c_str() : "Select...";
            if (ImGui::BeginCombo("##MidiIn", preview)) {
                for (int i = 0; i < (int)midiInNames.size(); ++i) {
                    bool selected = (i == selectedMidiIdx);
                    if (ImGui::Selectable(midiInNames[i].c_str(), selected)) {
                        selectedMidiIdx = i;
                        OpenMidiInPort((unsigned int)selectedMidiIdx);
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        ImGui::Separator();
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);

        // ---------------- 右栏：上波形，下日志 ----------------
        ImGui::BeginChild("RightPane", ImVec2(w2, 0), ImGuiChildFlags_Borders);
        float availy = ImGui::GetContentRegionAvail().y;
        float h1 = availy * 0.5f;

        // 右上：波形
        ImGui::BeginChild("RightUpperPane", ImVec2(0, h1), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        ImGui::Text("Audio Waveform");
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            ImGui::PlotLines("##wave",
                             audioBuffer.data(),
                             (int)audioBuffer.size(),
                             0, nullptr,
                             -1.0f, 1.0f,
                             ImVec2(ImGui::GetContentRegionAvail().x, h1 - 30));
        }
        ImGui::EndChild();

        // 右下：MIDI 日志
        ImGui::BeginChild("RightDownPane", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        ImGui::Text("MIDI Log (last 10):");
        {
            std::lock_guard<std::mutex> lk(midiMutex);
            for (auto& s : midiLog) ImGui::TextUnformatted(s.c_str());
        }
        ImGui::EndChild();

        ImGui::EndChild(); // RightPane

        ImGui::End();      // MainHost
        ImGui::PopStyleVar(2);

        // ---------------- 渲染 ----------------
        ImGui::Render();
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // 清理
    CloseAudio();
    if (gMidiIn.isPortOpen()) gMidiIn.closePort();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
