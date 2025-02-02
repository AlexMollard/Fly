#pragma once

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
	void RenderSpatialControl();

	std::string FormatTime(double seconds);

private:
	std::unique_ptr<MP3Player> m_mp3Player;
	bool m_show_file_dialog;
	std::string m_selected_file;
	FileDialog m_dialog;

	// Spatial audio animation state
    bool m_isRotating = false;
    bool m_isFigure8 = false;
    float m_rotationStartTime = 0.0f;
    float m_figure8StartTime = 0.0f;
    float m_lastRotationAngle = 0.0f;
};
