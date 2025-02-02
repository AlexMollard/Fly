#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class FileDialog
{
private:
	bool& m_show_dialog;
	std::string& m_selected_file;
	std::filesystem::path m_current_dir;
	std::string m_filter;
	char m_search_buffer[256] = "";
	std::vector<std::string> m_allowed_extensions;
	std::unordered_map<std::string, std::string> m_file_icons;

	// Cache for directory contents
	std::vector<std::filesystem::directory_entry> m_cached_entries;
	std::filesystem::file_time_type m_last_cache_update;

	void InitializeIcons();
	void UpdateDirectoryCache();
	bool IsFileTypeAllowed(const std::filesystem::path& path) const;
	void RenderPathBar();
	void RenderSearchBar();
	void RenderFileList();

public:
	FileDialog(bool& show_dialog, std::string& selected_file, const std::vector<std::string>& allowed_extensions = { ".mp3" });
	void Render();
};
