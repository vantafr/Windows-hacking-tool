#include "ui.hh"
#include "../globals.hh"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <sstream>
#include <TlHelp32.h>
#include <psapi.h>
#include <fstream>
#include <memory>
#include <thread>
#include <chrono>

// License checking and handling
void License() {
    std::string keyWithPrefix = globals.key;
    const std::string prefix = "KEYAUTH-";
    if (keyWithPrefix.find(prefix) != 0) {
        keyWithPrefix = prefix + keyWithPrefix;
    }

    KeyAuthApp.license(keyWithPrefix);

    if (KeyAuthApp.data.success) {
        for (std::string value : KeyAuthApp.data.subscriptions) globals.subs += value + " ";
        globals.tab = 1;
    }
    else {
        ImGui::Text("Login failed: %s", KeyAuthApp.data.message.c_str());
    }
}

// Show update message when the app is being updated
void show_update_message() {
    std::cout << "Sorry, users. Wukey is currently working on updating the software." << std::endl;
    std::cout << "Please come back later. Thank you!" << std::endl;
    std::cout << "Press any key to continue..." << std::endl;
    std::cin.get();
}

// UI initialization function
void ui::init(LPDIRECT3DDEVICE9 device) {
    dev = device;

    ShellExecute(NULL, "open", "https://guns.lol/wukey_temp", NULL, NULL, SW_SHOWNORMAL);

    KeyAuthApp.init();

    if (!KeyAuthApp.data.success) {
        show_update_message();
        return;
    }

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_TitleBg] = ImColor(30, 30, 45);
    style.Colors[ImGuiCol_TitleBgActive] = ImColor(30, 30, 45);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(30, 30, 45);

    style.Colors[ImGuiCol_WindowBg] = ImColor(21, 21, 33);

    style.Colors[ImGuiCol_Tab] = ImColor(24, 125, 228);
    style.Colors[ImGuiCol_TabActive] = ImColor(54, 153, 255);
    style.Colors[ImGuiCol_TabHovered] = ImColor(24, 125, 228);
    style.Colors[ImGuiCol_Button] = ImColor(54, 153, 255);

    if (window_pos.x == 0) {
        RECT screen_rect{};
        GetWindowRect(GetDesktopWindow(), &screen_rect);
        screen_res = ImVec2(float(screen_rect.right), float(screen_rect.bottom));
        window_pos = (screen_res - window_size) * 0.5f;
    }
}

// Function to load the icon from file (as .ico)
IDirect3DTexture9* iconTexture = nullptr;

bool loadIconTexture(const char* filePath, LPDIRECT3DDEVICE9 device) {
    // Load the ICO image using Windows API
    HICON hIcon = (HICON)LoadImage(NULL, filePath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    if (!hIcon) {
        std::cerr << "Failed to load icon!" << std::endl;
        return false;
    }

    ICONINFO iconInfo;
    if (!GetIconInfo(hIcon, &iconInfo)) {
        std::cerr << "Failed to get icon info!" << std::endl;
        return false;
    }

    // Get the icon size
    BITMAP bmp;
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);
    int iconWidth = bmp.bmWidth;
    int iconHeight = bmp.bmHeight;

    // Prepare the icon pixels (we'll directly use icon's color bitmap as a raw buffer)
    void* pPixels = nullptr;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = iconWidth;
    bmi.bmiHeader.biHeight = iconHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;  // 32-bit RGBA

    // Extract pixel data
    HDC hdc = CreateCompatibleDC(NULL);
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pPixels, NULL, 0);
    SelectObject(hdc, hBitmap);
    GetDIBits(hdc, iconInfo.hbmColor, 0, iconHeight, pPixels, &bmi, DIB_RGB_COLORS);
    DeleteDC(hdc);

    // Create a Direct3D texture from the raw icon pixels
    if (pPixels != nullptr) {
        HRESULT hr = device->CreateTexture(iconWidth, iconHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &iconTexture, NULL);
        if (FAILED(hr)) {
            std::cerr << "Failed to create texture!" << std::endl;
            return false;
        }

        // Copy the pixel data into the texture
        D3DLOCKED_RECT lockedRect;
        if (SUCCEEDED(iconTexture->LockRect(0, &lockedRect, NULL, D3DLOCK_DISCARD))) {
            memcpy(lockedRect.pBits, pPixels, iconWidth * iconHeight * 4);  // 4 bytes per pixel for RGBA
            iconTexture->UnlockRect(0);
        }
    }

    // Clean up
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);

    return true;
}

// UI rendering function
void ui::render() {
    if (!globals.active) return;

    // Load the icon texture if not already loaded (example usage)
    static bool iconLoaded = false;
    if (!iconLoaded) {
        iconLoaded = loadIconTexture("C:\\Users\\vanta\\Downloads\\iconws.ico", dev);  // Path to your icon file
    }

    if (!KeyAuthApp.data.success) {
        ImGui::SetNextWindowPos(ImVec2(window_pos.x, window_pos.y), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(window_size.x, window_size.y));
        ImGui::SetNextWindowBgAlpha(1.0f);

        ImGui::Begin(window_title, &globals.active, window_flags);
        {
            ImGui::Text("Error: %s", KeyAuthApp.data.message.c_str());
            ImGui::End();
        }

        std::cout << "Error: " << KeyAuthApp.data.message.c_str() << std::endl;
        return;
    }

    switch (globals.tab) {
    case 0: {
        ImGui::SetNextWindowPos(ImVec2(window_pos.x, window_pos.y), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(window_size.x, window_size.y));
        ImGui::SetNextWindowBgAlpha(1.0f);

        ImGui::Begin(window_title, &globals.active, window_flags);
        {
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
                if (ImGui::BeginTabItem("License Key")) {
                    ImGui::Text("Login using key only.");
                    ImGui::Separator();
                    ImGui::InputText("Key", globals.key, IM_ARRAYSIZE(globals.key));
                    if (ImGui::Button("Login")) {
                        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)License, NULL, NULL, NULL);
                    }

                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
            ImGui::SetCursorPosY(280);
            ImGui::Separator();
            ImGui::Text(KeyAuthApp.data.message.c_str());
        }
        ImGui::End();
    }
          break;

    case 1: {
        ImGui::SetNextWindowPos(ImVec2(window_pos.x, window_pos.y), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2({ 500, 500 })); // Adjusted window size for added features
        ImGui::SetNextWindowBgAlpha(1.0f);

        ImGui::Begin(window_title, &globals.active, window_flags);
        {
            // Display the icon image if loaded
            if (iconTexture != nullptr) {
                ImGui::Image((void*)iconTexture, ImVec2(64, 64)); // Display the icon (scaled to 64x64)
            }

            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
                if (ImGui::BeginTabItem("User Data")) {
                    if (!KeyAuthApp.data.subscriptions.empty()) {
                        std::string subscription = KeyAuthApp.data.subscriptions[0];
                        ImGui::TextColored(ImColor(0, 255, 0), "Welcome %s", subscription.c_str());
                    }
                    else {
                        ImGui::TextColored(ImColor(255, 0, 0), "No subscriptions found.");
                    }

                    ImGui::Text("IP Address: %s", KeyAuthApp.data.ip.c_str());
                    ImGui::Text("Hardware ID: %s", KeyAuthApp.data.hwid.c_str());

                    if (ImGui::Button("Temp Spoof")) {
                        KeyAuthApp.data.ip = "192.168.1.100";
                        KeyAuthApp.data.hwid = "D4F6B9A0-0349-4172-A3F9-42F8F4BB85F9";
                    }

                    if (ImGui::Button("Back")) {
                        globals.tab = 0;
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("System")) {
                    static bool networkFixed = false;
                    static bool performanceFixed = false;
                    static bool stickyKeysDisabled = false;
                    static bool ntfsCompressionDisabled = false;
                    static bool smb1Disabled = false;
                    static bool smb2Disabled = false;
                    static bool nvidiaFixed = false;
                    static bool driversExcluded = false;

                    if (ImGui::Button("Fix Network")) {
                        std::system("ipconfig /flushdns");
                        std::system("netsh winsock reset");
                        std::system("netsh int ip reset");
                        networkFixed = true;
                        std::cout << "Network optimized successfully.\n";
                    }

                    if (ImGui::Button("Fix Performance")) {
                        std::system("powercfg -restoredefaultschemes");
                        performanceFixed = true;
                        std::cout << "Performance optimized successfully.\n";
                    }

                    if (ImGui::Checkbox("Disable Sticky Keys", &stickyKeysDisabled)) {
                        if (stickyKeysDisabled) {
                            std::system("reg add \"HKEY_CURRENT_USER\\Control Panel\\Accessibility\\StickyKeys\" /v Flags /t REG_SZ /d 506 /f");
                            std::cout << "Sticky Keys disabled.\n";
                        }
                        else {
                            std::system("reg add \"HKEY_CURRENT_USER\\Control Panel\\Accessibility\\StickyKeys\" /v Flags /t REG_SZ /d 0 /f");
                            std::cout << "Sticky Keys restored.\n";
                        }
                    }

                    if (ImGui::Checkbox("Disable NTFS Compression", &ntfsCompressionDisabled)) {
                        if (ntfsCompressionDisabled) {
                            std::system("fsutil behavior set disablecompression 1");
                            std::cout << "NTFS compression disabled.\n";
                        }
                        else {
                            std::system("fsutil behavior set disablecompression 0");
                            std::cout << "NTFS compression restored.\n";
                        }
                    }

                    if (ImGui::Button("Disable SMBv1")) {
                        std::system("sc config lanmanworkstation start= disabled");
                        std::system("sc stop lanmanworkstation");
                        smb1Disabled = true;
                        std::cout << "SMBv1 disabled.\n";
                    }

                    if (ImGui::Button("Disable SMBv2")) {
                        std::system("sc config mrxsmb20 start= disabled");
                        std::system("sc stop mrxsmb20");
                        smb2Disabled = true;
                        std::cout << "SMBv2 disabled.\n";
                    }

                    if (ImGui::Button("Fix NVIDIA")) {
                        std::system("sc config NvTelemetryContainer start= disabled");
                        std::system("sc stop NvTelemetryContainer");
                        nvidiaFixed = true;
                        std::cout << "NVIDIA Telemetry disabled.\n";
                    }

                    if (ImGui::Button("Exclude Drivers from Updates")) {
                        DWORD excludeValue = 1;
                        LONG result = RegSetKeyValueA(
                            HKEY_LOCAL_MACHINE,
                            "SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate",
                            "ExcludeWUDriversInQualityUpdate",
                            REG_DWORD,
                            &excludeValue,
                            sizeof(excludeValue)
                        );

                        if (result == ERROR_SUCCESS) {
                            driversExcluded = true;
                            std::cout << "Driver updates excluded successfully.\n";
                        }
                        else {
                            driversExcluded = false;
                            std::cerr << "Failed to exclude driver updates. Error code: " << result << "\n";
                        }
                    }

                    // "Check Status" Button
                    if (ImGui::Button("Check Status")) {
                        std::ofstream statusFile("status.bat");
                        if (statusFile.is_open()) {
                            statusFile << "@echo off\n";
                            statusFile << "color 0A\n"; // Green background, bright text
                            statusFile << "echo System Status:\n";
                            statusFile << (networkFixed ? "echo \033[32mNetwork Fix: Applied\033[0m\n" : "echo \033[31mNetwork Fix: Not Applied\033[0m\n");
                            statusFile << (performanceFixed ? "echo \033[32mPerformance Fix: Applied\033[0m\n" : "echo \033[31mPerformance Fix: Not Applied\033[0m\n");
                            statusFile << (stickyKeysDisabled ? "echo \033[32mSticky Keys: Disabled\033[0m\n" : "echo \033[31mSticky Keys: Enabled\033[0m\n");
                            statusFile << (ntfsCompressionDisabled ? "echo \033[32mNTFS Compression: Disabled\033[0m\n" : "echo \033[31mNTFS Compression: Enabled\033[0m\n");
                            statusFile << (smb1Disabled ? "echo \033[32mSMBv1: Disabled\033[0m\n" : "echo \033[31mSMBv1: Enabled\033[0m\n");
                            statusFile << (smb2Disabled ? "echo \033[32mSMBv2: Disabled\033[0m\n" : "echo \033[31mSMBv2: Enabled\033[0m\n");
                            statusFile << (nvidiaFixed ? "echo \033[32mNVIDIA Telemetry: Fixed\033[0m\n" : "echo \033[31mNVIDIA Telemetry: Not Fixed\033[0m\n");
                            statusFile << (driversExcluded ? "echo \033[32mDriver Updates: Excluded\033[0m\n" : "echo \033[31mDriver Updates: Not Excluded\033[0m\n");
                            statusFile << "pause\n";
                            statusFile.close();

                            std::system("start status.bat");
                        }
                    }

                    ImGui::EndTabItem();
                }

            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
          break;


    }
}
