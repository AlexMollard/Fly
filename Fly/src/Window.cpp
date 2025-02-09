#include "pch.h"

#include "Window.h"

#include <hello_imgui/hello_imgui.h>
#include <imgui_internal.h>

Window::Window(HelloImGui::RunnerParams& params)
      : m_dialog(m_showFileDialog, m_selectedFile), m_roomReverb(m_audioStreamer.GetRoomReverb())
{
	params.callbacks.SetupImGuiStyle = [this]() { GuiSetup(); };

	m_tonalityControl.SetBass(0.0f);   // Neutral bass (-1 to 1)
	m_tonalityControl.SetTreble(0.0f); // Neutral treble (-1 to 1)
	m_audioStreamer.SetEffectProcessor(m_tonalityControl.CreateProcessor());
	m_tonalityControl.SetSource(m_audioStreamer.GetSource());
}

void Window::Update()
{
	if (m_viusalizerEnabled)
	{
		m_audioStreamer.Update();
	}
}

void Window::Render()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::Begin("MP3 Player##MainWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	// Calculate main layout dimensions
	float windowWidth = ImGui::GetWindowWidth() - 3 * ImGui::GetStyle().WindowPadding.x;
	float windowHeight = ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y;
	float bottomPanelHeight = 155.0f; // Height for progress bar and controls
	float mainContentHeight = windowHeight - bottomPanelHeight - ImGui::GetStyle().ItemSpacing.y * 2;
	float playlistWidth = windowWidth * 0.3f; // 30% of window width

	// Main content area (Playlist + Controls)
	RenderPlaylistPanel(playlistWidth, mainContentHeight + ImGui::GetStyle().ItemSpacing.y);
	ImGui::SameLine();
	ImGui::SetCursorPosY(ImGui::GetStyle().WindowPadding.y);

	if (m_showFileDialog)
	{
		m_dialog.Render(mainContentHeight);
		if (!m_selectedFile.empty())
		{
			m_audioStreamer.OpenFromFile(m_selectedFile);
			m_audioStreamer.Play();
			m_playlist.AddTrack(m_selectedFile);
			m_selectedFile.clear();
		}
	}
	else
	{
		RenderControlsPanel(mainContentHeight);
	}

	// Bottom panel (Progress bar and playback controls)
	ImGui::SetCursorPosY(mainContentHeight + ImGui::GetStyle().ItemSpacing.y * 2);
	RenderBottomPanel(bottomPanelHeight);

	ImGui::End();
}

void Window::RenderPlaylistPanel(float width, float height)
{
	// Calculate the heights
	float controlsHeight = height * 0.125f;         // 12% of window height
	float playlistHeight = height - controlsHeight; // Remaining 90%

	// Main playlist panel
	ImGui::BeginChild("Playlist", ImVec2(width, playlistHeight - ImGui::GetStyle().ItemSpacing.y * 2), true);

	ImGui::Text("Playlist");
	ImGui::Separator();

	const auto& tracks = m_playlist.GetTracks();
	for (size_t i = 0; i < tracks.size(); i++)
	{
		std::string filename = std::filesystem::path(tracks[i]).filename().string();
		// Remove the file extension
		filename = filename.substr(0, filename.find_last_of('.'));

		// Selectable track
		if (ImGui::Selectable(filename.c_str(), m_playlist.GetCurrentIndex() == i))
		{
			m_audioStreamer.OpenFromFile(tracks[i]);
			m_audioStreamer.Play();
		}
	}
	ImGui::EndChild();

	// Playlist controls
	ImGui::BeginChild("PlaylistControls", ImVec2(width, controlsHeight), true);

	float buttonWidth = (width - ImGui::GetStyle().ItemSpacing.x * 4) * 0.5f;
	float buttonHeight = controlsHeight - ImGui::GetStyle().ItemSpacing.y * 3;

	if (ImGui::Button((ICON_LC_PLUS "##AddFile"), ImVec2(buttonWidth, buttonHeight)))
	{
		m_showFileDialog = true;
	}

	ImGui::SameLine();
	if (ImGui::Button((ICON_LC_MINUS "##RemoveFile"), ImVec2(buttonWidth, buttonHeight)))
	{
		size_t lastIndex = m_playlist.Size() - 1;

		if (m_playlist.GetCurrentIndex() == lastIndex)
		{
			m_audioStreamer.Stop();
		}

		m_playlist.RemoveTrack(lastIndex);
	}

	ImGui::EndChild();
}

void Window::RenderControlsPanel(float height)
{
	ImGui::BeginChild("Controls", ImVec2(0, height), true);

	if (m_audioStreamer.GetStatus() != AudioStreamer::Status::Stopped)
	{
		RenderTrackInfo();
		RenderVisualizer();
		RenderAudioFilters();
		RenderRoomProps();
		RenderSpatialControl();
	}
	else
	{
		// Center the text vertically in the panel
		float textHeight = ImGui::GetTextLineHeightWithSpacing() * 4; // Height for 4 lines of text
		float startY = (height - textHeight) * 0.5f;
		ImGui::SetCursorPosY(startY);

		// Center align the text
		float windowWidth = ImGui::GetContentRegionAvail().x;

		// Welcome text
		const char* welcomeText = ICON_LC_MUSIC "  Welcome to the Fly Audio Player";
		float textWidth = ImGui::CalcTextSize(welcomeText).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_ButtonActive], "%s", welcomeText);

		ImGui::Spacing();
		ImGui::Spacing();

		// Instructions text
		const char* instructionsText = "To begin, click the '+' button in the playlist panel.";
		textWidth = ImGui::CalcTextSize(instructionsText).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text("%s", instructionsText);

		ImGui::Spacing();
		ImGui::Spacing();

		// Supported formats text
		const char* formatsText = "Supported formats: MP3 (To come: WAV, OGG, FLAC)";
		textWidth = ImGui::CalcTextSize(formatsText).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled], "%s", formatsText);
	}

	ImGui::EndChild();
}

void Window::RenderBottomPanel(float height)
{
	ImGui::BeginChild("BottomPanel", ImVec2(0, height), true);

	// Ensure we have a track loaded
	if (m_audioStreamer.GetStatus() != AudioStreamer::Status::Stopped)
	{
		// Progress section
		ImGui::BeginGroup();
		RenderProgressBar();
		RenderTimeDisplay();
		ImGui::EndGroup();

		// Playback controls to the right and volume control to the left
		ImGui::BeginGroup();
		RenderPlaybackControls();
		ImGui::EndGroup();

		RenderVolumeControl();
	}

	ImGui::EndChild();
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

	LOG_DEBUG("GUI Setup complete");
}

void Window::RenderTrackInfo()
{
	const AudioStreamer::TrackInfo& trackInfo = m_audioStreamer.GetTrackInfo();
	ImGuiStyle& style = ImGui::GetStyle();

	// Now Playing header with accent color
	ImGui::TextColored(style.Colors[ImGuiCol_HeaderActive], "NOW PLAYING");
	ImGui::Separator();

	// Helper lambda to display field if not empty
	auto displayField = [&](const char* label, const std::string& value)
	{
		if (!value.empty())
		{
			ImGui::TextColored(style.Colors[ImGuiCol_TextDisabled], label); // Secondary text color
			ImGui::SameLine();
			ImGui::TextColored(style.Colors[ImGuiCol_Text], "%s", value.c_str()); // Primary text color
		}
	};

	// Display non-empty fields
	displayField("Title:", trackInfo.title);
	displayField("Artist:", trackInfo.artist);
	displayField("Album:", trackInfo.album);
	displayField("Genre:", trackInfo.genre);
	displayField("Year:", trackInfo.year);
}

void Window::RenderProgressBar()
{
	static float lastProgress = 0.0f;
	static bool isDragging = false;
	static float dragProgress = 0.0f;

	double currentTime = m_audioStreamer.GetPlayingOffset();
	float duration = m_audioStreamer.GetDuration();

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
		float seekTime = (dragProgress / 100.0f) * duration;
		m_audioStreamer.SetPlayingOffset(seekTime);
		lastProgress = dragProgress;
	}

	ImGui::PopStyleColor();
}

void Window::RenderTimeDisplay()
{
	// Get current playback time and total duration
	double currentTime = m_audioStreamer.GetPlayingOffset();
	double duration = m_audioStreamer.GetDuration();

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
		m_playlist.Previous();
		m_audioStreamer.OpenFromFile(m_playlist.GetCurrentTrack());
	}
	ImGui::SameLine();
	if (ImGui::Button(m_audioStreamer.GetStatus() == AudioStreamer::Status::Playing ? ("  " ICON_LC_PAUSE "  ") : ("  " ICON_LC_PLAY "  "), ImVec2(controlWidth, buttonHeight)))
	{
		if (m_audioStreamer.GetStatus() == AudioStreamer::Status::Playing)
			m_audioStreamer.Pause();
		else
			m_audioStreamer.Play();
	}
	ImGui::SameLine();
	if (ImGui::Button(("  " ICON_LC_SQUARE "  "), ImVec2(controlWidth, buttonHeight)))
	{
		m_audioStreamer.Stop(true);
	}
	ImGui::SameLine();
	if (ImGui::Button(("  " ICON_LC_SKIP_FORWARD "  "), ImVec2(controlWidth, buttonHeight)))
	{
		m_playlist.Next();
		m_audioStreamer.OpenFromFile(m_playlist.GetCurrentTrack());
	}
	ImGui::SameLine();
	// Change the color of the shuffle button if active
	ImGui::PushStyleColor(ImGuiCol_Button, m_playlist.IsShuffleEnabled() ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
	if (ImGui::Button(("  " ICON_LC_SHUFFLE "  "), ImVec2(controlWidth, buttonHeight)))
	{
		m_playlist.ToggleShuffle();
	}
	ImGui::PopStyleColor();

	ImGui::PopStyleVar();
}

void Window::RenderVolumeControl()
{
	float volume = m_audioStreamer.GetVolume() * 100.0f;

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
		m_audioStreamer.setVolume(volume / 100.0f);
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
	// Start a group for the filters
	ImGui::BeginGroup();

	ImGui::Separator();
	ImGui::Text(ICON_LC_RADIO "  Sound Properties");

	// Calculate the widest label to align all sliders
	float maxLabelWidth = 0.0f;
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_AUDIO_WAVEFORM "  Bass").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_ACTIVITY "  Treble").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_FAST_FORWARD "  Pitch").x);
	maxLabelWidth += ImGui::GetStyle().ItemSpacing.x; // Add some padding

	// Bass Control
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_AUDIO_WAVEFORM "  Bass");
	ImGui::SameLine(maxLabelWidth);
	float bass = m_tonalityControl.GetBass();
	// scale the value to 0 to 100
	bass = (bass + 1.0f) * 50.0f;
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##Bass", &bass, 0.0f, 100.0f, "%.0f%%"))
	{
		// scale it to -1 to 1
		bass = (bass - 50.0f) / 50.0f;
		m_tonalityControl.SetBass(bass);
	}

	// Treble Control
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_ACTIVITY "  Treble");
	ImGui::SameLine(maxLabelWidth);
	float treble = m_tonalityControl.GetTreble();
	// scale the value to 0 to 100
	treble = (treble + 1.0f) * 50.0f;
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##Treble", &treble, 0.0f, 100.0f, "%.0f%%"))
	{
		// scale it to -1 to 1
		treble = (treble - 50.0f) / 50.0f;
		m_tonalityControl.SetTreble(treble);
	}

	// Pitch Control
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_FAST_FORWARD "  Pitch");
	ImGui::SameLine(maxLabelWidth);
	float pitch = m_tonalityControl.GetPitch();
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##Pitch", &pitch, -12.0f, 12.0f, "%.1f semitones"))
	{
		m_tonalityControl.SetPitch(pitch);
	}

	// Presets Section
	ImGui::Spacing();

	// Calculate button width to fit 6 buttons
	float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 5) / 6;

	// First row of preset buttons
	if (ImGui::Button("Default", ImVec2(buttonWidth, 0)))
	{
		m_tonalityControl.SetDefaultPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Slowed", ImVec2(buttonWidth, 0)))
	{
		m_tonalityControl.SetSlowedPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Chipmunk", ImVec2(buttonWidth, 0)))
	{
		m_tonalityControl.SetChipmunkPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Deep", ImVec2(buttonWidth, 0)))
	{
		m_tonalityControl.SetDeepPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Radio", ImVec2(buttonWidth, 0)))
	{
		m_tonalityControl.SetRadioPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Random", ImVec2(buttonWidth, 0)))
	{
		m_tonalityControl.SetRandomPreset();
	}

	ImGui::EndGroup();
}

void Window::RenderRoomProps()
{
	ImGui::BeginGroup();

	ImGui::Separator();
	ImGui::Text(ICON_LC_WAVES "  Reverb Properties");

	// Calculate max label width for alignment
	float maxLabelWidth = 0.0f;
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_HOURGLASS "  Decay Time").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_WAVES "  Early Refl.").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_TIMER "  Late Reverb").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_EXPAND "  Room Size").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_CLOCK "  Refl. Delay").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_BELL_RING "  Late Delay").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_DROPLETS "  Dampening").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_WIND "  Air Absorp.").x);
	maxLabelWidth = max(maxLabelWidth, ImGui::CalcTextSize(ICON_LC_RADIUS "  Rolloff").x);
	maxLabelWidth += ImGui::GetStyle().ItemSpacing.x;

	// Decay Time
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_HOURGLASS "  Decay Time");
	ImGui::SameLine(maxLabelWidth);
	float decayTime = m_roomReverb.GetDecayTime();
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##DecayTime", &decayTime, 0.1f, 20.0f, "%.1f s"))
	{
		m_roomReverb.SetDecayTime(decayTime);
	}

	// Early Reflections Gain
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_WAVES "  Early Refl.");
	ImGui::SameLine(maxLabelWidth);
	float reflGain = m_roomReverb.GetReflectionsGain();
	float reflPercentage = reflGain * 31.6f; // 3.16 = 100%
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##ReflGain", &reflPercentage, 0.0f, 100.0f, "%.0f%%"))
	{
		m_roomReverb.SetReflectionsGain(reflPercentage / 31.6f);
	}

	// Reflections Delay
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_CLOCK "  Refl. Delay");
	ImGui::SameLine(maxLabelWidth);
	float reflDelay = m_roomReverb.GetReflectionsDelay();
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##ReflDelay", &reflDelay, 0.0f, 0.3f, "%.3f s"))
	{
		m_roomReverb.SetReflectionsDelay(reflDelay);
	}

	// Late Reverb Gain
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_TIMER "  Late Reverb");
	ImGui::SameLine(maxLabelWidth);
	float lateGain = m_roomReverb.GetLateGain();
	float latePercentage = lateGain * 10.0f;
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##LateGain", &latePercentage, 0.0f, 100.0f, "%.0f%%"))
	{
		m_roomReverb.SetLateGain(latePercentage / 10.0f);
	}

	// Late Reverb Delay
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_BELL_RING "  Late Delay");
	ImGui::SameLine(maxLabelWidth);
	float lateDelay = m_roomReverb.GetLateDelay();
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##LateDelay", &lateDelay, 0.0f, 0.1f, "%.3f s"))
	{
		m_roomReverb.SetLateDelay(lateDelay);
	}

	// HF Dampening (using HF Ratio)
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_DROPLETS "  Dampening");
	ImGui::SameLine(maxLabelWidth);
	float hfRatio = m_roomReverb.GetDecayHFRatio();
	float dampPercentage = (hfRatio - 0.1f) * 52.6f;
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##HFRatio", &dampPercentage, 0.0f, 100.0f, "%.0f%%"))
	{
		m_roomReverb.SetDecayHFRatio(0.1f + (dampPercentage / 52.6f));
	}

	// Air Absorption
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_WIND "  Air Absorp.");
	ImGui::SameLine(maxLabelWidth);
	float airAbsorption = m_roomReverb.GetAirAbsorption();
	float airPercentage = (airAbsorption - 0.892f) * 926.0f;
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##AirAbsorption", &airPercentage, 0.0f, 100.0f, "%.0f%%"))
	{
		m_roomReverb.SetAirAbsorption(0.892f + (airPercentage / 926.0f));
	}

	// Room Rolloff
	ImGui::Spacing();
	ImGui::AlignTextToFramePadding();
	ImGui::Text(ICON_LC_RADIUS "  Rolloff");
	ImGui::SameLine(maxLabelWidth);
	float rolloff = m_roomReverb.GetRoomRolloff();
	float rolloffPercentage = rolloff * 10.0f;
	ImGui::SetNextItemWidth(-1);
	if (ImGui::SliderFloat("##Rolloff", &rolloffPercentage, 0.0f, 100.0f, "%.0f%%"))
	{
		m_roomReverb.SetRoomRolloff(rolloffPercentage / 10.0f);
	}

	// Presets
	ImGui::Spacing();

	// Calculate button width to fit 6 buttons
	float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 5) / 6;

	if (ImGui::Button("Reset", ImVec2(buttonWidth, 0)))
	{
		m_roomReverb.SetDefaultPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Small", ImVec2(buttonWidth, 0)))
	{
		m_roomReverb.SetSmallRoomPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Medium", ImVec2(buttonWidth, 0)))
	{
		m_roomReverb.SetMediumRoomPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Large", ImVec2(buttonWidth, 0)))
	{
		m_roomReverb.SetLargeRoomPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Hall", ImVec2(buttonWidth, 0)))
	{
		m_roomReverb.SetHallPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cave", ImVec2(buttonWidth, 0)))
	{
		m_roomReverb.SetCavePreset();
	}

	ImGui::EndGroup();
}

void Window::RenderVisualizer()
{
	m_viusalizerEnabled = false;
	if (ImGui::CollapsingHeader("Visualizer##CollapsingHeader"))
	{
		m_viusalizerEnabled = true;
		float VISUALIZER_HEIGHT = 100.0f;
		const float MIN_BAR_HEIGHT = 2.0f;
		const float BAR_SPACING = 2.0f; // Increased spacing

		ImGui::BeginChild("Visualizer", ImVec2(0, VISUALIZER_HEIGHT), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		const std::vector<float>& vizData = m_audioStreamer.GetVisualizerData();
		const std::vector<float>& peakData = m_audioStreamer.GetBandPeaks();

		if (vizData.empty())
		{
			ImGui::EndChild();
			return;
		}

		VISUALIZER_HEIGHT -= ImGui::GetStyle().WindowPadding.y * 2;

		// Calculate dimensions
		ImVec2 contentRegion = ImGui::GetContentRegionAvail();
		float availWidth = contentRegion.x;
		float barWidth = (availWidth - (BAR_SPACING * (vizData.size() - 1))) / vizData.size();
		float centerY = VISUALIZER_HEIGHT / 2.0f;

		// Get colors from ImGui style
		const ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 barColor = style.Colors[ImGuiCol_ButtonActive];
		ImVec4 peakColor = ImVec4(barColor.x * 1.2f, barColor.y * 1.2f, barColor.z * 1.2f, 1.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(BAR_SPACING, 0));

		for (size_t i = 0; i < vizData.size(); i++)
		{
			// Apply non-linear scaling for better visual dynamics
			float value = std::pow(vizData[i], 0.7f); // Adjust power for different curve
			float peak = std::pow(peakData[i], 0.7f);

			float rawHeight = value * (VISUALIZER_HEIGHT * 0.95f);
			float height = max(rawHeight, MIN_BAR_HEIGHT);
			float halfHeight = height / 2.0f;

			ImGui::PushID(static_cast<int>(i));

			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			// Calculate vertical positions
			float topY = cursorPos.y + centerY - halfHeight;
			float bottomY = cursorPos.y + centerY + halfHeight;

			// Draw gradient bar
			ImVec4 gradientTop = ImVec4(barColor.x * 1.2f, barColor.y * 1.2f, barColor.z * 1.2f, barColor.w);
			ImVec4 gradientBottom = barColor;

			drawList->AddRectFilledMultiColor(ImVec2(cursorPos.x, topY), ImVec2(cursorPos.x + barWidth, bottomY), ImGui::GetColorU32(gradientTop), ImGui::GetColorU32(gradientTop), ImGui::GetColorU32(gradientBottom), ImGui::GetColorU32(gradientBottom));

			// Draw peak indicator
			float peakHeight = peak * (VISUALIZER_HEIGHT * 0.95f);
			float peakY = cursorPos.y + centerY - (peakHeight / 2.0f);
			drawList->AddRectFilled(ImVec2(cursorPos.x, peakY), ImVec2(cursorPos.x + barWidth, peakY + 1.0f), ImGui::GetColorU32(peakColor));

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
}

void Window::RenderSpatialControl()
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text(ICON_LC_MOVE_3D "  Spatial Audio Control");
	ImGui::Spacing();

	// Calculate available space
	ImVec2 availSpace = ImGui::GetContentRegionAvail();
	float gridSize = min(availSpace.x * 0.6f, 200.0f); // Take 60% of width, cap at 200px

	// Create a horizontal layout
	ImGui::Columns(2, "SpatialControlLayout", false);

	// Left column: Grid
	ImGui::SetColumnWidth(0, gridSize + ImGui::GetStyle().ItemSpacing.x);

	// Create a child window for the 2D grid
	ImGui::BeginChild("SpatialGrid", ImVec2(gridSize, gridSize));

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize(gridSize, gridSize);

	// Draw grid background
	drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(40, 40, 40, 255));

	// Draw grid lines
	for (int i = 1; i < 4; i++)
	{
		float linePos = i * (canvasSize.x / 4);
		// Vertical lines
		drawList->AddLine(ImVec2(canvasPos.x + linePos, canvasPos.y), ImVec2(canvasPos.x + linePos, canvasPos.y + canvasSize.y), IM_COL32(70, 70, 70, 255));
		// Horizontal lines
		drawList->AddLine(ImVec2(canvasPos.x, canvasPos.y + linePos), ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + linePos), IM_COL32(70, 70, 70, 255));
	}

	// Get current positions
	auto sourcePos = m_audioStreamer.GetPosition();
	auto listenerPos = m_audioStreamer.GetListenerPosition();

	// Convert from OpenAL coordinates (-1 to 1) to screen coordinates
	ImVec2 sourceScreenPos(canvasPos.x + (sourcePos.first + 1.0f) * 0.5f * canvasSize.x, canvasPos.y + (sourcePos.second + 1.0f) * 0.5f * canvasSize.y);
	ImVec2 listenerScreenPos(canvasPos.x + (listenerPos.first + 1.0f) * 0.5f * canvasSize.x, canvasPos.y + (listenerPos.second + 1.0f) * 0.5f * canvasSize.y);

	// Draw source point (orange)
	drawList->AddCircleFilled(sourceScreenPos, 6.0f, IM_COL32(255, 128, 0, 255));
	drawList->AddText(ImVec2(sourceScreenPos.x + 10, sourceScreenPos.y - 10), IM_COL32(255, 128, 0, 255), "Source");

	// Draw listener point (white)
	drawList->AddCircleFilled(listenerScreenPos, 6.0f, IM_COL32(255, 255, 255, 255));
	drawList->AddText(ImVec2(listenerScreenPos.x + 10, listenerScreenPos.y - 10), IM_COL32(255, 255, 255, 255), "Listener");

	// Handle dragging
	ImGui::InvisibleButton("canvas", canvasSize);
	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
	{
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 relativePos((mousePos.x - canvasPos.x) / canvasSize.x, (mousePos.y - canvasPos.y) / canvasSize.y);

		float x = (relativePos.x * 2.0f - 1.0f);
		float z = (relativePos.y * 2.0f - 1.0f);

		m_audioStreamer.SetPosition(x, z);
	}

	ImGui::EndChild();

	// Right column: Movement controls
	ImGui::NextColumn();

	ImGui::Text("Movement Patterns");
	ImGui::Spacing();

	// Calculate button width based on available space in the right column
	float buttonWidth = ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x * 2;
	float buttonHeight = 35.0f;

	// Stack buttons vertically
	if (ImGui::Button((ICON_LC_ROTATE_3D "  Rotate"), ImVec2(buttonWidth, buttonHeight)))
	{
		m_isRotating = !m_isRotating;
		if (m_isRotating)
		{
			m_rotationStartTime = static_cast<float>(ImGui::GetTime());
			m_lastRotationAngle = 0.0f;
		}
	}

	ImGui::Spacing();

	if (ImGui::Button((ICON_LC_INFINITY "  Figure-8"), ImVec2(buttonWidth, buttonHeight)))
	{
		m_isFigure8 = !m_isFigure8;
		if (m_isFigure8)
		{
			m_figure8StartTime = static_cast<float>(ImGui::GetTime());
			m_isRotating = false;
		}
	}

	ImGui::Spacing();

	if (ImGui::Button((ICON_LC_UNDO_2 "  Reset"), ImVec2(buttonWidth, buttonHeight)))
	{
		m_audioStreamer.SetPosition(0.0f, 0.0f);
		m_isRotating = false;
		m_isFigure8 = false;
	}

	ImGui::Columns(1);

	// Update positions based on animations
	if (m_isRotating)
	{
		float currentTime = static_cast<float>(ImGui::GetTime());
		float elapsedTime = currentTime - m_rotationStartTime;
		float angle = elapsedTime * 1.0f;

		float radius = 0.5f;
		float x = radius * cos(angle);
		float z = radius * sin(angle);

		m_audioStreamer.SetPosition(x, z);
		m_lastRotationAngle = angle;
	}
	else if (m_isFigure8)
	{
		float currentTime = static_cast<float>(ImGui::GetTime());
		float t = (currentTime - m_figure8StartTime) * 0.5f;

		float scale = 0.5f;
		float x = scale * cos(t);
		float z = scale * sin(2 * t) * 0.5f;

		m_audioStreamer.SetPosition(x, z);
	}
}
