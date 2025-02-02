#include "FileDialog.h"

#include <imgui.h>

#include "IconsLucide.h"
#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#endif

void FileDialog::InitializeIcons()
{
	m_file_icons = {
		{ ".mp3", ICON_LC_MUSIC},
        { ".wav", ICON_LC_MUSIC},
        {".flac", ICON_LC_MUSIC},
        { ".m4a", ICON_LC_MUSIC},
 // Add more file type icons as needed
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
	// Drive selection for Windows
#ifdef _WIN32
	char drives[MAX_PATH] = { 0 };
	DWORD drive_strings_length = GetLogicalDriveStringsA(MAX_PATH, drives);
	if (drive_strings_length > 0)
	{
		if (ImGui::BeginCombo("##Drives", m_current_dir.root_name().string().c_str()))
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
			ImGui::EndCombo();
		}
		ImGui::SameLine();
	}
#endif

	std::filesystem::path temp = m_current_dir;
	std::vector<std::filesystem::path> paths;
	while (temp.has_parent_path())
	{
		if (!paths.empty())
		{
			if (temp == paths.back())
			{
				break;
			}
		}

		paths.push_back(temp);
		temp = temp.parent_path();
	}

	for (auto it = paths.rbegin(); it != paths.rend(); ++it)
	{
		if (it != paths.rbegin())
		{
			ImGui::SameLine();
			ImGui::TextUnformatted("/");
			ImGui::SameLine();
		}
		if (ImGui::Button(it->filename().string().empty() ? it->root_name().string().c_str() : it->filename().string().c_str()))
		{
			m_current_dir = *it;
		}
	}
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

	// Parent directory button
	if (m_current_dir.has_parent_path())
	{
		if (ImGui::Selectable(std::string(ICON_LC_FOLDER "  ..").c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if (ImGui::IsMouseDoubleClicked(0))
			{
				m_current_dir = m_current_dir.parent_path();
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

		try
		{
			if (entry.is_directory())
			{
				if (ImGui::Selectable((ICON_LC_FOLDER "  " + filename).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
				{
					if (ImGui::IsMouseDoubleClicked(0))
					{
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

				if (ImGui::Selectable((icon + "  " + filename).c_str(), false))
				{
					m_selected_file = path.string();
					m_show_dialog = false;
				}

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text("Size: %.2f KB", static_cast<float>(std::filesystem::file_size(path)) / 1024.0f);
					ImGui::EndTooltip();
				}
			}
		}
		catch ([[maybe_unused]] const std::filesystem::filesystem_error& e)
		{
			// Skip files that can't be accessed
			continue;
		}
	}

	ImGui::EndChild();
}

FileDialog::FileDialog(bool& show_dialog, std::string& selected_file, const std::vector<std::string>& allowed_extensions)
      : m_show_dialog(show_dialog), m_selected_file(selected_file), m_current_dir(std::filesystem::current_path()), m_allowed_extensions(allowed_extensions)
{
	InitializeIcons();
}

void FileDialog::Render()
{
	if (!m_show_dialog)
		return;

	ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("File Explorer##dialog", &m_show_dialog))
	{
		RenderPathBar();
		ImGui::Separator();
		RenderSearchBar();
		RenderFileList();
	}
	ImGui::End();
}
