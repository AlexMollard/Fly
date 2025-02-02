#include "Window.h"

#include <hello_imgui/hello_imgui.h>

Window::Window(HelloImGui::RunnerParams& params)
      : m_mp3Player(std::make_unique<MP3Player>()), m_show_file_dialog(false), m_dialog(m_show_file_dialog, m_selected_file)
{
	params.callbacks.BeforeImGuiRender = [this]() { GuiSetup(); };
}

void Window::Update()
{
	m_mp3Player->update();
}

void Window::Render()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::Begin("MP3 Player##MainWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	RenderPlaylistPanel();
	ImGui::SameLine();
	ImGui::SetCursorPosY(ImGui::GetStyle().WindowPadding.y);
	RenderControlsPanel();

	// Show file dialog
	if (m_show_file_dialog)
	{
		m_dialog.Render();
		if (!m_selected_file.empty())
		{
			m_mp3Player->loadTrack(m_selected_file);
			m_mp3Player->play();
			m_selected_file.clear();
		}
	}

	ImGui::End();
}

void Window::GuiSetup()
{
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	// Gruvbox Colors
	const ImVec4 bg0_hard = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);     // #1d2021
	const ImVec4 bg0 = ImVec4(0.156f, 0.156f, 0.156f, 1.0f);    // #282828
	const ImVec4 bg1 = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);          // #3c3836
	const ImVec4 bg2 = ImVec4(0.237f, 0.219f, 0.207f, 1.0f);    // #504945
	const ImVec4 orange = ImVec4(0.956f, 0.517f, 0.235f, 1.0f); // #fe8019
	const ImVec4 red = ImVec4(0.984f, 0.286f, 0.203f, 1.0f);    // #fb4934
	const ImVec4 green = ImVec4(0.721f, 0.733f, 0.149f, 1.0f);  // #b8bb26
	const ImVec4 yellow = ImVec4(0.984f, 0.756f, 0.235f, 1.0f); // #fabd2f
	const ImVec4 blue = ImVec4(0.513f, 0.647f, 0.596f, 1.0f);   // #83a598
	const ImVec4 aqua = ImVec4(0.556f, 0.752f, 0.486f, 1.0f);   // #8ec07c
	const ImVec4 fg1 = ImVec4(0.952f, 0.858f, 0.698f, 1.0f);    // #ebdbb2
	const ImVec4 fg2 = ImVec4(0.901f, 0.831f, 0.639f, 1.0f);    // #d5c4a1

	// Styling
	style.WindowRounding = 4.0f;
	style.ChildRounding = 4.0f;
	style.PopupRounding = 4.0f;
	style.ScrollbarRounding = 4.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;

	style.WindowPadding = ImVec2(12, 12);
	style.ScrollbarSize = 12.0f;
	style.SelectableTextAlign = ImVec2(0.0f, 0.5f);
	style.FramePadding = ImVec2(6, 6);
	style.ItemSpacing = ImVec2(8, 8);
	style.ItemInnerSpacing = ImVec2(4, 4);
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.GrabMinSize = 10.0f;
	style.FrameRounding = 4.0f;

	// Colors
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] = bg0_hard;
	colors[ImGuiCol_PopupBg] = bg0;

	// Frames
	colors[ImGuiCol_FrameBg] = bg1;
	colors[ImGuiCol_FrameBgHovered] = bg2;
	colors[ImGuiCol_FrameBgActive] = orange * ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

	// Buttons
	colors[ImGuiCol_Button] = bg2;
	colors[ImGuiCol_ButtonHovered] = orange;
	colors[ImGuiCol_ButtonActive] = orange * ImVec4(1.2f, 1.2f, 1.2f, 1.0f);

	// Headers
	colors[ImGuiCol_Header] = orange * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	colors[ImGuiCol_HeaderHovered] = orange;
	colors[ImGuiCol_HeaderActive] = orange * ImVec4(1.2f, 1.2f, 1.2f, 1.0f);

	// Title
	colors[ImGuiCol_TitleBg] = bg0_hard;
	colors[ImGuiCol_TitleBgActive] = bg1;
	colors[ImGuiCol_TitleBgCollapsed] = bg0 * ImVec4(1.0f, 1.0f, 1.0f, 0.75f);

	// Tabs
	colors[ImGuiCol_Tab] = bg2;
	colors[ImGuiCol_TabHovered] = orange;
	colors[ImGuiCol_TabActive] = orange * ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
	colors[ImGuiCol_TabUnfocused] = bg1;
	colors[ImGuiCol_TabUnfocusedActive] = bg2;

	// Sliders/Scrollbars
	colors[ImGuiCol_SliderGrab] = orange * ImVec4(1.0f, 1.0f, 1.0f, 0.9f);
	colors[ImGuiCol_SliderGrabActive] = orange * ImVec4(1.2f, 1.2f, 1.2f, 1.0f);
	colors[ImGuiCol_ScrollbarBg] = bg0 * ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
	colors[ImGuiCol_ScrollbarGrab] = bg2;
	colors[ImGuiCol_ScrollbarGrabHovered] = orange * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	colors[ImGuiCol_ScrollbarGrabActive] = orange;

	// Text
	colors[ImGuiCol_Text] = fg1;
	colors[ImGuiCol_TextDisabled] = fg2 * ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

	// Borders and separators
	colors[ImGuiCol_Border] = bg2;
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered] = orange * ImVec4(1.0f, 1.0f, 1.0f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = orange;
}

void Window::RenderPlaylistPanel()
{
	// Calculate the heights
	float totalHeight = ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y * 3;
	float controlsHeight = totalHeight * 0.1f;           // 10% of window height
	float playlistHeight = totalHeight - controlsHeight; // Remaining 90%
	float panelWidth = ImGui::GetWindowWidth() * 0.3f;

	// Main playlist panel
	ImGui::BeginChild("Playlist", ImVec2(panelWidth, playlistHeight), true);

	ImGui::Text("Playlist");
	ImGui::Separator();

	const auto& playlist = m_mp3Player->getPlaylist();
	for (size_t i = 0; i < playlist.size(); i++)
	{
		std::string filename = std::filesystem::path(playlist[i]).filename().string();
		// Remove the file extension
		filename = filename.substr(0, filename.find_last_of('.'));

		// Selectable track
		if (ImGui::Selectable(filename.c_str(), m_mp3Player->getCurrentTrack() == playlist[i]))
		{
			m_mp3Player->loadTrack(playlist[i]);
			m_mp3Player->play();
		}
	}
	ImGui::EndChild();

	// Playlist controls - now using 10% of window height
	ImGui::BeginChild("PlaylistControls", ImVec2(panelWidth, controlsHeight), true);

	// Calculate positioning for centered buttons
	float buttonWidth = (panelWidth - ImGui::GetStyle().ItemSpacing.x * 4) * 0.5f;
	float buttonHeight = controlsHeight - ImGui::GetStyle().ItemSpacing.y * 3;

	// Add button
	if (ImGui::Button((ICON_LC_PLUS "##AddFile"), ImVec2(buttonWidth, buttonHeight)))
	{
		m_show_file_dialog = true;
	}

	// Remove button
	ImGui::SameLine();
	if (ImGui::Button((ICON_LC_MINUS "##RemoveFile"), ImVec2(buttonWidth, buttonHeight)))
	{
		if (!playlist.empty())
		{
			size_t lastIndex = playlist.size() - 1;
			m_mp3Player->removeTrack(lastIndex);
		}
	}

	ImGui::EndChild();
}

void Window::RenderControlsPanel()
{
	// Calculate the heights
	float totalHeight = ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y * 2.375f;

	ImGui::BeginChild("Controls", ImVec2(0, totalHeight), true);

	RenderTrackInfo();
	RenderPlaybackControls();
	RenderVolumeControl();
	RenderAudioFilters();

	ImGui::EndChild();
}

void Window::RenderTrackInfo()
{
	std::string filename = std::filesystem::path(m_mp3Player->getCurrentTrack()).filename().string();
	ImGui::TextWrapped("Now Playing:");
	ImGui::TextWrapped("%s", filename.empty() ? "-" : filename.c_str());

	RenderVisualizer();
	RenderProgressBar();
	RenderTimeDisplay();
}

void Window::RenderProgressBar()
{
	static float lastProgress = 0.0f;
	static bool isDragging = false;
	static float dragProgress = 0.0f;

	double currentTime = m_mp3Player->getCurrentTime();
	double duration = m_mp3Player->getDuration();

	// Calculate current progress and round to 1 decimal place to reduce jitter
	float progress = (duration > 0.0) ? static_cast<float>((currentTime / duration) * 100.0f) : 0.0f;
	progress = std::round(progress * 10.0f) / 10.0f; // Round to nearest 0.1
	progress = std::clamp(progress, 0.0f, 100.0f);

	// Only update last progress if not dragging
	if (!isDragging)
	{
		lastProgress = progress;
		dragProgress = progress;
	}

	// Display progress - use drag progress while dragging, actual progress otherwise
	float displayProgress = isDragging ? dragProgress : lastProgress;

	// Start the slider
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	ImGui::SetNextItemWidth(-1);

	if (ImGui::SliderFloat("##Progress", &dragProgress, 0.0f, 100.0f, ""))
	{
		if (!isDragging)
		{
			isDragging = true;
		}
	}

	// Handle drag state
	if (ImGui::IsItemActive())
	{
		isDragging = true;
	}
	else if (isDragging)
	{
		// User finished dragging - update position
		isDragging = false;
		m_mp3Player->setCurrentTime(dragProgress);
		lastProgress = dragProgress;
	}

	ImGui::PopStyleColor();
}

void Window::RenderTimeDisplay()
{
	// Get current playback time and total duration
	double currentTime = m_mp3Player->getCurrentTime();
	double duration = m_mp3Player->getDuration();

	// Format both times
	std::string currentTimeStr = FormatTime(currentTime);
	std::string durationStr = FormatTime(duration);

	// Display formatted time
	ImGui::Text("%s / %s", currentTimeStr.c_str(), durationStr.c_str());
}

void Window::RenderPlaybackControls()
{
	float controlWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 4) / 5;
	float buttonHeight = 35;

	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

	if (ImGui::Button(("  " ICON_LC_SKIP_BACK "  "), ImVec2(controlWidth, buttonHeight)))
	{
		m_mp3Player->playPrevious();
	}
	ImGui::SameLine();
	if (ImGui::Button(m_mp3Player->getIsPlaying() ? ("  " ICON_LC_PAUSE "  ") : ("  " ICON_LC_PLAY "  "), ImVec2(controlWidth, buttonHeight)))
	{
		if (m_mp3Player->getIsPlaying())
			m_mp3Player->pause();
		else
			m_mp3Player->play();
	}
	ImGui::SameLine();
	if (ImGui::Button(("  " ICON_LC_SQUARE "  "), ImVec2(controlWidth, buttonHeight)))
	{
		m_mp3Player->stop();
	}
	ImGui::SameLine();
	if (ImGui::Button(("  " ICON_LC_SKIP_FORWARD "  "), ImVec2(controlWidth, buttonHeight)))
	{
		m_mp3Player->playNext();
	}
	ImGui::SameLine();
	if (ImGui::Button(("  " ICON_LC_SHUFFLE "  "), ImVec2(controlWidth, buttonHeight)))
	{
		m_mp3Player->toggleShuffle();
	}

	ImGui::PopStyleVar();
}

void Window::RenderVolumeControl()
{
	float volume = m_mp3Player->getVolume();

	// Start a group to ensure consistent alignment
	ImGui::BeginGroup();

	// Volume icon with proper spacing
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_VOLUME);
	ImGui::SameLine(0, 8.0f); // Add specific spacing after icon

	// Volume slider
	ImGui::SetNextItemWidth(-1); // Take remaining width
	if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 100.0f, "%.0f%%"))
	{
		m_mp3Player->setVolume(volume);
	}

	ImGui::EndGroup();
}

std::string Window::FormatTime(double seconds)
{
	int totalSeconds = static_cast<int>(seconds);
	int minutes = totalSeconds / 60;
	int remainingSeconds = totalSeconds % 60;

	std::stringstream ss;
	ss << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << remainingSeconds;
	return ss.str();
}

void Window::RenderAudioFilters()
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Start a group for the filters
	ImGui::BeginGroup();

	// Calculate the widest label to align all sliders
	float maxLabelWidth = 0.0f;
	maxLabelWidth = std::max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_AUDIO_WAVEFORM "  LowPass").x);
	maxLabelWidth = std::max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_ACTIVITY "  HighPass").x);
	maxLabelWidth = std::max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_FAST_FORWARD "  Pitch").x);
	maxLabelWidth += ImGui::GetStyle().ItemSpacing.x; // Add some padding

	// LowPass Control (50Hz - 2000Hz)
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_AUDIO_WAVEFORM "  LowPass");
	ImGui::SameLine(maxLabelWidth);
	float bass = m_mp3Player->getBass();
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##Bass", &bass, 0.0f, 100.0f, "%.0f%%"))
	{
		m_mp3Player->setBass(bass);
	}

	ImGui::Spacing();

	// HighPass Control (2000Hz - 20000Hz)
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_ACTIVITY "  HighPass");
	ImGui::SameLine(maxLabelWidth);
	float treble = m_mp3Player->getTreble();
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##Treble", &treble, 0.0f, 100.0f, "%.0f%%"))
	{
		m_mp3Player->setTreble(treble);
	}

	// Pitch Control
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_FAST_FORWARD "  Pitch");
	ImGui::SameLine(maxLabelWidth);
	float pitch = m_mp3Player->getPitch();
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##Pitch", &pitch, 0.0f, 100.0f, "%.0f%%"))
	{
		m_mp3Player->setPitch(pitch);
	}
	// Preset Buttons
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Fun Presets");
	ImGui::Spacing();

	float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3;

	if (ImGui::Button("Chipmunk Mode", ImVec2(buttonWidth, 35)))
	{
		m_mp3Player->setPitch(50.0f);
		m_mp3Player->setTreble(100.0f);
		m_mp3Player->setBass(0.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Slowed", ImVec2(buttonWidth, 35)))
	{
		m_mp3Player->setPitch(25.0f);
		m_mp3Player->setBass(35.0f);
		m_mp3Player->setTreble(0.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset", ImVec2(buttonWidth, 35)))
	{
		m_mp3Player->setPitch(50.0f);
		m_mp3Player->setTreble(0.0f);
		m_mp3Player->setBass(100.0f);
	}

	ImGui::EndGroup();
}

void Window::RenderVisualizer()
{
	float VISUALIZER_HEIGHT = 100.0f;
	const float MIN_BAR_HEIGHT = 2.0f;
	const float BAR_SPACING = 1.0f;

	ImGui::BeginChild("Visualizer", ImVec2(0, VISUALIZER_HEIGHT), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const std::vector<float>& vizData = m_mp3Player->getVisualizerData();
	if (vizData.empty())
	{
		ImGui::EndChild();
		return;
	}

	VISUALIZER_HEIGHT -= ImGui::GetStyle().WindowPadding.y * 2;

	// Get color from ImGui style
	const ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 barColor = style.Colors[ImGuiCol_ButtonActive];

	// Calculate dimensions
	ImVec2 contentRegion = ImGui::GetContentRegionAvail();
	float availWidth = contentRegion.x;
	float barWidth = (availWidth - (BAR_SPACING * (vizData.size() - 1))) / vizData.size();
	float centerY = VISUALIZER_HEIGHT / 2.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(BAR_SPACING, 0));

	for (size_t i = 0; i < vizData.size(); i++)
	{
		float rawHeight = vizData[i] * (VISUALIZER_HEIGHT * 0.95f);
		float height = std::max(rawHeight, MIN_BAR_HEIGHT);
		float halfHeight = height / 2.0f;

		ImGui::PushID(static_cast<int>(i));

		// Draw centered bar
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Calculate vertical positions for centered bar
		float topY = cursorPos.y + centerY - halfHeight;
		float bottomY = cursorPos.y + centerY + halfHeight;

		// Draw simple solid bar
		drawList->AddRectFilled(ImVec2(cursorPos.x, topY), ImVec2(cursorPos.x + barWidth, bottomY), ImGui::GetColorU32(barColor));

		ImGui::Dummy(ImVec2(barWidth, VISUALIZER_HEIGHT));

		if (i < vizData.size() - 1)
		{
			ImGui::SameLine();
		}

		ImGui::PopID();
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();
}
