#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <filesystem>
#include <imgui.h>
#include <memory>
#include <string>

#include "FileDialog.h"
#include "IconsLucide.h"
#include "MP3Player.h"

namespace HelloImGui
{
	struct RunnerParams;
}

class Window
{
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

	std::string FormatTime(double seconds);

private:
	std::unique_ptr<MP3Player> m_mp3Player;
	bool m_show_file_dialog;
	std::string m_selected_file;
	FileDialog m_dialog;
};
