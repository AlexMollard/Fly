#pragma once

#include "FileDialog.h"
#include "IconsLucide.h"
#include "MP3Streamer.h"
#include "PlayList.h"
#include "TonalityControl.h"

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
	// void RenderVisualizer();
	// void RenderSpatialControl();

	std::string FormatTime(double seconds);

private:
	std::unique_ptr<MP3Streamer> m_audioStreamer;
	std::unique_ptr<Playlist> m_playlist;
	TonalityControl m_tonalityControl;

	bool m_showFileDialog = false;
	std::string m_selectedFile;
	FileDialog m_dialog;

	// Spatial audio animation state
    bool m_isRotating = false;
    bool m_isFigure8 = false;
    float m_rotationStartTime = 0.0f;
    float m_figure8StartTime = 0.0f;
    float m_lastRotationAngle = 0.0f;
};
