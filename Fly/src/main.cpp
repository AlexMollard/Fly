﻿#include <filesystem>
#include <hello_imgui/hello_imgui.h>
#include <imgui.h>
#include <string>
#include <string_view>
#include <tchar.h>
#include <windows.h>

#include "Window.h"

constexpr std::string_view WINDOW_TITLE = "Fly Player";
constexpr HelloImGui::ScreenSize WINDOW_SIZE{ 900, 600 };
constexpr float BASE_FONT_SIZE = 16.0f;

// Font configuration
struct FontConfig
{
	std::string path;
	float size;
	const ImWchar* ranges;
	ImFontConfig* config;
};

bool LoadFont(const FontConfig& fontConfig, ImFontAtlas* fonts)
{
	if (!std::filesystem::exists(fontConfig.path))
	{
		return false;
	}
	return fonts->AddFontFromFileTTF(fontConfig.path.c_str(), fontConfig.size, fontConfig.config, fontConfig.ranges) != nullptr;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	HelloImGui::RunnerParams params;

	// Window configuration
	params.appWindowParams.windowTitle = WINDOW_TITLE;
	params.appWindowParams.windowGeometry.size = WINDOW_SIZE;
	params.appWindowParams.windowGeometry.sizeAuto = false;

	// Docking configuration
	params.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
	params.imGuiWindowParams.showMenuBar = false;
	params.imGuiWindowParams.showStatusBar = false;

	// Setup callbacks
	params.callbacks.LoadAdditionalFonts = []()
	{
		const std::string resDir = IsDebuggerPresent() ? "../res/" : "";
		ImGuiIO& io = ImGui::GetIO();

		// Regular font configuration
		FontConfig regularFont{ resDir + "JetBrainsMono.ttf", BASE_FONT_SIZE, nullptr, nullptr };

		// Icon font configuration
		static const ImWchar icons_ranges[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.OversampleH = 2;
		icons_config.OversampleV = 2;
		icons_config.GlyphOffset = ImVec2(0, 3);

		FontConfig iconFont{ resDir + "lucide.ttf", BASE_FONT_SIZE, icons_ranges, &icons_config };

		if (!LoadFont(regularFont, io.Fonts))
		{
			MessageBox(nullptr, _T("Failed to load regular font"), _T("Error"), MB_OK | MB_ICONERROR);
		}
		if (!LoadFont(iconFont, io.Fonts))
		{
			MessageBox(nullptr, _T("Failed to load icon font"), _T("Error"), MB_OK | MB_ICONERROR);
		}

		io.FontGlobalScale = 1.0f;
	};

	// Create MP3 player window
	auto playerWindow = std::make_unique<Window>(params);

	// Main GUI callback
	params.callbacks.ShowGui = [&playerWindow]()
	{
		if (playerWindow)
		{
			playerWindow->Update();
			playerWindow->Render();
		}
	};

	HelloImGui::Run(params);
	return 0;
}
