#pragma once
#include <d3d9.h>
#include "../imgui/imgui.h"

namespace ui {


    void FixPerformance();
    void FixNetwork();
    void DisableSystemRestore();
    void FixNTFS();
    void DisableSMBv1();
    void DisableSMBv2();
    void DisableStickyKeys();
    void CleanTemporaryFiles();
    void CleanBSODLogs();
    void CleanLogs();
    void AnalyzeSystem();
    void ChangeNetworkAdapter(int selectedAdapter);



    // Function declarations
    void init(LPDIRECT3DDEVICE9 device);  // Initialize with Direct3D device
    void render();  // Render function for UI

    // Direct3D device and other global variables for UI
    inline LPDIRECT3DDEVICE9 dev;
    inline const char* window_title = "[WIN11] Tool made by Wukey";

    // Window dimensions and flags
    inline ImVec2 screen_res{ 000, 000 };
    inline ImVec2 window_pos{ 0, 0 };
    inline ImVec2 window_size{ 400, 300 };
    inline DWORD window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

}  // namespace ui
