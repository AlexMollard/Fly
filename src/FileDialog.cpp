#include "pch.h"

#include "FileDialog.h"

#include "IconsLucide.h"

FileDialog::FileDialog(bool& show_dialog, std::string& selected_file, const std::vector<std::string>& allowed_extensions)
      : m_show_dialog(show_dialog), m_selected_file(selected_file), m_current_dir(std::filesystem::current_path()), m_allowed_extensions(allowed_extensions)
{
	InitializeIcons();

	// Add the root path to the history
	AddToHistory(m_current_dir);
}

// I might move this to like a config file or something not sure just yet
void FileDialog::InitializeIcons()
{
	m_file_icons = {
		// Audio files
		{   ".mp3",            ICON_LC_MUSIC },
		{   ".wav",            ICON_LC_MUSIC },
		{  ".flac",            ICON_LC_MUSIC },
		{   ".m4a",            ICON_LC_MUSIC },
		{   ".aac",            ICON_LC_MUSIC },
		{   ".ogg",            ICON_LC_MUSIC },
		{   ".wma",            ICON_LC_MUSIC },

		// Video files
		{   ".mp4",            ICON_LC_VIDEO },
		{   ".mov",            ICON_LC_VIDEO },
		{   ".avi",            ICON_LC_VIDEO },
		{   ".mkv",            ICON_LC_VIDEO },
		{   ".wmv",            ICON_LC_VIDEO },
		{   ".flv",            ICON_LC_VIDEO },
		{  ".webm",            ICON_LC_VIDEO },

		// Image files
		{   ".jpg",            ICON_LC_IMAGE },
		{  ".jpeg",            ICON_LC_IMAGE },
		{   ".png",            ICON_LC_IMAGE },
		{   ".gif",            ICON_LC_IMAGE },
		{   ".bmp",            ICON_LC_IMAGE },
		{   ".svg",            ICON_LC_IMAGE },
		{  ".webp",            ICON_LC_IMAGE },
		{  ".tiff",            ICON_LC_IMAGE },

		// Document files
		{   ".pdf",        ICON_LC_FILE_TEXT },
		{   ".doc",        ICON_LC_FILE_TEXT },
		{  ".docx",        ICON_LC_FILE_TEXT },
		{   ".txt",        ICON_LC_FILE_TEXT },
		{   ".rtf",        ICON_LC_FILE_TEXT },
		{   ".odt",        ICON_LC_FILE_TEXT },

		// Spreadsheet files
		{   ".xls", ICON_LC_FILE_SPREADSHEET },
		{  ".xlsx", ICON_LC_FILE_SPREADSHEET },
		{   ".csv", ICON_LC_FILE_SPREADSHEET },
		{   ".ods", ICON_LC_FILE_SPREADSHEET },

		// Archive files
		{   ".zip",          ICON_LC_ARCHIVE },
		{   ".rar",          ICON_LC_ARCHIVE },
		{    ".7z",          ICON_LC_ARCHIVE },
		{   ".tar",          ICON_LC_ARCHIVE },
		{    ".gz",          ICON_LC_ARCHIVE },

		// Code files
		{   ".cpp",             ICON_LC_CODE },
		{     ".h",             ICON_LC_CODE },
		{    ".py",             ICON_LC_CODE },
		{    ".js",             ICON_LC_CODE },
		{  ".html",             ICON_LC_CODE },
		{   ".css",             ICON_LC_CODE },
		{  ".json",             ICON_LC_CODE },
		{   ".xml",             ICON_LC_CODE },

		// Font files
		{   ".ttf",             ICON_LC_TYPE },
		{   ".otf",             ICON_LC_TYPE },
		{  ".woff",             ICON_LC_TYPE },
		{ ".woff2",             ICON_LC_TYPE },

		// Executable files
		{   ".exe",         ICON_LC_TERMINAL },
		{   ".msi",         ICON_LC_TERMINAL },
		{   ".app",         ICON_LC_TERMINAL },
		{    ".sh",         ICON_LC_TERMINAL },
		{   ".bat",         ICON_LC_TERMINAL }
	};
}

void FileDialog::UpdateDirectoryCache()
{
	try
	{
		auto current_time = std::filesystem::last_write_time(m_current_dir);
		if (current_time != m_last_cache_update)
		{
			m_cached_entries.clear();
			for (const auto& entry: std::filesystem::directory_iterator(m_current_dir))
			{
				m_cached_entries.push_back(entry);
			}
			m_last_cache_update = current_time;
		}
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
	}
}

bool FileDialog::IsFileTypeAllowed(const std::filesystem::path& path) const
{
	if (m_allowed_extensions.empty())
		return true;
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return std::find(m_allowed_extensions.begin(), m_allowed_extensions.end(), ext) != m_allowed_extensions.end();
}

void FileDialog::RenderPathBar()
{
	ImGui::BeginGroup();

	// Drive selection for Windows with consistent height
#ifdef _WIN32
	float button_height = ImGui::GetFrameHeight();
	ImGui::SetNextItemWidth(60); // Fixed width for drive dropdown

	if (ImGui::BeginCombo("##Drives", m_current_dir.root_name().string().c_str(), ImGuiComboFlags_HeightRegular))
	{
		char drives[MAX_PATH] = { 0 };
		DWORD drive_strings_length = GetLogicalDriveStringsA(MAX_PATH, drives);
		if (drive_strings_length > 0)
		{
			char* drive = drives;
			while (*drive)
			{
				if (ImGui::Selectable(drive))
				{
					m_current_dir = drive;
				}
				drive += strlen(drive) + 1;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine(0, 4);
#endif

	// Get available width for breadcrumbs
	float available_width = ImGui::GetContentRegionAvail().x;
	float spacing = 4.0f;
	float separator_width = ImGui::CalcTextSize("/").x + spacing * 2;
	float ellipsis_width = ImGui::CalcTextSize("...").x + spacing * 2;

	// Collect path components
	std::filesystem::path temp = m_current_dir;
	std::vector<std::pair<std::string, std::filesystem::path>> path_components;

	while (temp.has_parent_path())
	{
		if (!path_components.empty() && temp == path_components.back().second)
			break;

		std::string label = temp.filename().string().empty() ? temp.root_name().string() : temp.filename().string();

		path_components.push_back({ label, temp });
		temp = temp.parent_path();
	}

	// Calculate total width needed
	float total_width = 0;
	std::vector<float> component_widths;

	for (const auto& comp: path_components)
	{
		float width = ImGui::CalcTextSize(comp.first.c_str()).x + ImGui::GetStyle().FramePadding.x * 2; // Button padding
		component_widths.push_back(width);
		total_width += width + separator_width;
	}

	// Render breadcrumbs with smart truncation
	bool showing_ellipsis = false;
	float current_width = 0;
	int index = 0;

	// Always show root and last 2 components if possible
	size_t num_components = path_components.size();
	std::vector<bool> should_show(num_components, false);

	if (num_components > 0)
		should_show[0] = true; // Root
	if (num_components > 1)
		should_show[num_components - 1] = true; // Current directory
	if (num_components > 2)
		should_show[num_components - 2] = true; // Parent directory

	// Calculate space needed for mandatory components
	float mandatory_width = 0;
	for (size_t i = 0; i < num_components; i++)
	{
		if (should_show[i])
		{
			mandatory_width += component_widths[i] + separator_width;
		}
	}

	// Add middle components if space allows
	float remaining_width = available_width - mandatory_width;
	if (remaining_width > ellipsis_width && num_components > 3)
	{
		// Start from second component (index 1) and end before the last two components
		for (size_t i = 1; i < num_components - 2 && i < component_widths.size(); i++)
		{
			// Ensure we don't access beyond vector bounds
			if (i >= component_widths.size())
				break;

			float comp_width = component_widths[i] + separator_width;
			if (remaining_width >= comp_width + ellipsis_width)
			{
				should_show[i] = true;
				remaining_width -= comp_width;
			}
			else
			{
				break;
			}
		}
	}

	// Render components
	bool first = true;
	bool prev_shown = false;

	for (auto it = path_components.rbegin(); it != path_components.rend(); ++it)
	{
		size_t reverse_index = std::distance(path_components.rbegin(), it);
		size_t forward_index = num_components - 1 - reverse_index;

		ImGui::PushID(index++);

		if (!first && prev_shown)
		{
			ImGui::SameLine(0, spacing);
			ImGui::TextUnformatted("/");
			ImGui::SameLine(0, spacing);
		}

		if (should_show[forward_index])
		{
			if (!first && !prev_shown)
			{
				ImGui::SameLine(0, spacing);
				ImGui::TextUnformatted("...");
				ImGui::SameLine(0, spacing);
				ImGui::TextUnformatted("/");
				ImGui::SameLine(0, spacing);
			}

			if (ImGui::Button(it->first.c_str()))
			{
				m_current_dir = it->second;
			}
			prev_shown = true;
		}
		else
		{
			prev_shown = false;
		}

		first = false;
		ImGui::PopID();
	}

	ImGui::EndGroup();

	// On the far right a close button
	ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x * 2 - ImGui::GetFrameHeight());
	if (ImGui::Button(ICON_LC_X))
		m_show_dialog = false;
}

void FileDialog::RenderSearchBar()
{
	ImGui::SetNextItemWidth(-1);
	if (ImGui::InputTextWithHint("##Search", "Search files...", m_search_buffer, sizeof(m_search_buffer)))
	{
		m_filter = m_search_buffer;
		std::transform(m_filter.begin(), m_filter.end(), m_filter.begin(), ::tolower);
	}
}

void FileDialog::RenderFileList()
{
	ImGui::BeginChild("Files", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

	UpdateDirectoryCache();
	HandleKeyboardInput();
	HandleMouseInput();

	// Sort entries: directories first, then files
	std::vector<const std::filesystem::directory_entry*> sorted_entries;
	for (const auto& entry: m_cached_entries)
	{
		sorted_entries.push_back(&entry);
	}

	std::sort(sorted_entries.begin(),
	        sorted_entries.end(),
	        [](const std::filesystem::directory_entry* a, const std::filesystem::directory_entry* b)
	        {
		        if (a->is_directory() != b->is_directory())
		        {
			        return a->is_directory();
		        }
		        return a->path().filename() < b->path().filename();
	        });

	// Modify the file/directory rendering to show selection:
	int current_index = 0;

	// Parent directory button
	if (m_current_dir.has_parent_path())
	{
		if (ImGui::Selectable(std::string(ICON_LC_FOLDER "  ..").c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if (ImGui::IsMouseClicked(0))
			{
				NavigateUp();
			}
		}
	}

	// Display entries
	for (const auto* entry_ptr: sorted_entries)
	{
		const auto& entry = *entry_ptr;
		const auto& path = entry.path();
		std::string filename = path.filename().string();

		// Apply search filter
		if (!m_filter.empty())
		{
			std::string lower_filename = filename;
			std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
			if (lower_filename.find(m_filter) == std::string::npos)
			{
				continue;
			}
		}

		if (entry.is_directory())
		{
			if (ImGui::Selectable((ICON_LC_FOLDER "  " + filename).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
			{
				if (ImGui::IsMouseClicked(0))
				{
					AddToHistory(m_current_dir / filename);
					m_current_dir /= filename;
				}
			}
		}
		else if (IsFileTypeAllowed(path))
		{
			std::string ext = path.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			std::string icon = m_file_icons[ext];
			if (icon.empty())
				icon = ICON_LC_FILE;

			if (ImGui::Selectable((icon + "  " + filename).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
			{
				if (ImGui::IsMouseClicked(0))
				{
					m_selected_file = path.string();
					m_show_dialog = false;
				}
			}

			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Size: %.2f KB", static_cast<float>(std::filesystem::file_size(path)) / 1024.0f);
				ImGui::EndTooltip();
			}
		}
		else
		{
			std::string ext = path.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			std::string icon = m_file_icons[ext];
			if (icon.empty())
				icon = ICON_LC_FILE;

			ImGui::TextDisabled((icon + "  " + filename).c_str());
		}
		current_index++;
	}

	ImGui::EndChild();
}

void FileDialog::Render(float height)
{
	if (!m_show_dialog)
		return;

	ImGui::BeginChild("File Explorer##dialog", ImVec2(0, height), true);
	{
		// Toolbar and path area with proper spacing
		ImGui::BeginGroup();
		RenderToolbar();
		ImGui::SameLine(0, 8); // Add spacing between toolbar and path
		RenderPathBar();
		ImGui::EndGroup();

		ImGui::Separator();
		RenderSearchBar();
		RenderFileList();
	}
	ImGui::EndChild();
}

void FileDialog::HandleKeyboardInput()
{
	if (!ImGui::IsWindowFocused())
		return;

	// Refresh with F5
	if (ImGui::IsKeyPressed(ImGuiKey_F5))
	{
		UpdateDirectoryCache();
	}

	// Navigation shortcuts
	if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
	{
		NavigateUp();
	}

	if (ImGui::GetIO().KeyCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket))
		{
			NavigateBack();
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_RightBracket))
		{
			NavigateForward();
		}
	}

	// Focus search box
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F))
	{
		ImGui::SetKeyboardFocusHere(-1);
	}
}

void FileDialog::HandleMouseInput()
{
	// Mouse button 4 (back) and 5 (forward) support
	if (ImGui::IsMouseClicked(3)) // Mouse button 4
	{
		NavigateBack();
	}
	else if (ImGui::IsMouseClicked(4)) // Mouse button 5
	{
		NavigateForward();
	}
}

void FileDialog::NavigateBack()
{
	if (m_history.empty())
		return;

	if (m_history_position > 0)
	{
		m_history_position--;
		m_current_dir = m_history[m_history_position];
	}
}

void FileDialog::NavigateForward()
{
	if (m_history.empty())
		return;

	if (m_history_position < m_history.size() - 1)
	{
		m_history_position++;
		m_current_dir = m_history[m_history_position];
	}
}

void FileDialog::NavigateUp()
{
	if (m_current_dir.has_parent_path())
	{
		AddToHistory(m_current_dir.parent_path());
		m_current_dir = m_current_dir.parent_path();
	}
}

void FileDialog::AddToHistory(const std::filesystem::path& path)
{
	// Remove any forward history
	if (m_history_position < m_history.size())
	{
		m_history.erase(m_history.begin() + m_history_position + 1, m_history.end());
	}

	m_history.push_back(path);
	m_history_position = m_history.size() - 1;
}

void FileDialog::SelectItem(int index)
{
	if (index >= 0 && index < static_cast<int>(m_cached_entries.size()))
	{
		// Ensure the selected item is visible
		ImGui::SetScrollHereY(0.5f);
	}
}

void FileDialog::NavigateToHome()
{
	m_current_dir = std::filesystem::current_path();
}

void FileDialog::RenderToolbar()
{
	// Start horizontal group to ensure consistent spacing
	ImGui::BeginGroup();

	// Navigation buttons with consistent spacing
	if (ImGui::Button(ICON_LC_ARROW_LEFT))
		NavigateBack();
	ImGui::SameLine(0, 4);

	if (ImGui::Button(ICON_LC_ARROW_RIGHT))
		NavigateForward();
	ImGui::SameLine(0, 4);

	if (ImGui::Button(ICON_LC_ARROW_UP))
		NavigateUp();
	ImGui::SameLine(0, 4);

	if (ImGui::Button(ICON_LC_HOUSE))
		NavigateToHome();
	ImGui::SameLine(0, 4);

	if (ImGui::Button(ICON_LC_REFRESH_CW))
		UpdateDirectoryCache();

	ImGui::EndGroup();
}
