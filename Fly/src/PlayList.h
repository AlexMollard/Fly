#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Playlist
{
public:
	Playlist() = default;

	// Add/Remove tracks
	void addTrack(const std::string& filepath);
	void removeTrack(size_t index);
	void clear();

	// Track management
	bool next();
	bool previous();
	bool jumpToTrack(size_t index);

	// Playlist properties
	size_t size() const;
	bool isEmpty() const;

	// Current track info
	size_t getCurrentIndex() const;
	std::string getCurrentTrack() const;
	std::vector<std::string> getTracks() const;

	// Shuffle functionality
	void toggleShuffle();
	bool isShuffleEnabled() const;

private:
	std::vector<std::string> m_tracks;
	std::vector<size_t> m_shuffleIndices;
	size_t m_currentIndex = 0;
	bool m_shuffleEnabled = false;

	void updateShuffleIndices();
};
