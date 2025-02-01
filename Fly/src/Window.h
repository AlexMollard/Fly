#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <memory>
#include <string>
#include <filesystem>
#include "IconsLucide.h"
#include "MP3Player.h"
#include "FileDialog.h"

namespace HelloImGui {
	struct RunnerParams;
}

class Window {
public:
	Window(HelloImGui::RunnerParams& params);

	void Update();
	void Render();

private:
	void GuiSetup();

	void RenderPlaylistPanel();
	void RenderControlsPanel();
	void RenderTrackInfo();
	void RenderProgressBar();
	void RenderTimeDisplay();
	void RenderPlaybackControls();
	void RenderVolumeControl();
	void RenderAudioFilters();
	void RenderVisualizer();

	std::string FormatTime(float seconds);

private:
	std::unique_ptr<MP3Player> m_mp3Player;
	bool m_show_file_dialog;
	std::string m_selected_file;
	FileDialog m_dialog;
};
