#include <hello_imgui/hello_imgui.h>
#include <imgui.h>
#include "Window.h"

int main(int, char**)
{
    HelloImGui::RunnerParams params;

    // Window configuration
    params.appWindowParams.windowTitle = "Fly Player";
    params.appWindowParams.windowGeometry.size = { 900, 600 };
    params.appWindowParams.windowGeometry.sizeAuto = false;

    // Docking configuration
    params.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
    params.imGuiWindowParams.showMenuBar = false;
    params.imGuiWindowParams.showStatusBar = false;

    // Setup callbacks
    params.callbacks.LoadAdditionalFonts = []() {
        ImGuiIO& io = ImGui::GetIO();
        float baseFontSize = 16.0f;

        // Load the regular font
        io.Fonts->AddFontFromFileTTF("../res/JetBrainsMono.ttf", baseFontSize);

        // Load the icon font
        static const ImWchar icons_ranges[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = 2;
        icons_config.OversampleV = 2;
        icons_config.GlyphOffset = ImVec2(0, 3);
        io.Fonts->AddFontFromFileTTF("../res/lucide.ttf", baseFontSize, &icons_config, icons_ranges);

        io.FontGlobalScale = 1.f;
        };

    // Create MP3 player window
    auto playerWindow = std::make_unique<Window>(params);

    // Main GUI callback
    params.callbacks.ShowGui = [&]() {
        playerWindow->Render();
        };

    HelloImGui::Run(params);
    return 0;
}