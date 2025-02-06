#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Playlist
{
public:
	Playlist() = default;

	// Add/Remove tracks
	void AddTrack(const std::string& filepath);
	void RemoveTrack(size_t index);
	void Clear();

	// Track management
	bool Next();
	bool Previous();
	bool JumpToTrack(size_t index);

	// Playlist properties
	size_t Size() const;
	bool IsEmpty() const;

	// Current track info
	size_t GetCurrentIndex() const;
	std::string GetCurrentTrack() const;
	std::vector<std::string> GetTracks() const;

	// Shuffle functionality
	void ToggleShuffle();
	bool IsShuffleEnabled() const;

private:
	std::vector<std::string> m_tracks;
	std::vector<size_t> m_shuffleIndices;
	size_t m_currentIndex = 0;
	bool m_shuffleEnabled = false;

	void UpdateShuffleIndices();
};
