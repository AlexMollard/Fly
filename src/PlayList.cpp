#include "pch.h"

#include "Playlist.h"

#include <algorithm>
#include <random>
#include <numeric>

void Playlist::AddTrack(const std::string& filepath)
{
	m_tracks.push_back(filepath);
	if (m_shuffleEnabled)
	{
		UpdateShuffleIndices();
	}
}

void Playlist::RemoveTrack(size_t index)
{
	if (index >= m_tracks.size())
		return;

	// Adjust current index if necessary
	if (index < m_currentIndex)
	{
		m_currentIndex--;
	}
	else if (index == m_currentIndex && m_currentIndex == m_tracks.size() - 1)
	{
		m_currentIndex = max(0ul, m_currentIndex - 1);
	}

	m_tracks.erase(m_tracks.begin() + index);
	if (m_shuffleEnabled)
	{
		UpdateShuffleIndices();
	}
}

void Playlist::Clear()
{
	m_tracks.clear();
	m_shuffleIndices.clear();
	m_currentIndex = 0;
}

bool Playlist::Next()
{
	if (m_tracks.empty())
		return false;

	if (m_shuffleEnabled)
	{
		auto it = std::find(m_shuffleIndices.begin(), m_shuffleIndices.end(), m_currentIndex);
		if (it != m_shuffleIndices.end() && it + 1 != m_shuffleIndices.end())
		{
			m_currentIndex = *(it + 1);
			return true;
		}
		return false;
	}
	else
	{
		if (m_currentIndex + 1 < m_tracks.size())
		{
			m_currentIndex++;
			return true;
		}
		return false;
	}
}

bool Playlist::Previous()
{
	if (m_tracks.empty() || m_currentIndex == 0)
		return false;

	if (m_shuffleEnabled)
	{
		auto it = std::find(m_shuffleIndices.begin(), m_shuffleIndices.end(), m_currentIndex);
		if (it != m_shuffleIndices.begin())
		{
			m_currentIndex = *(it - 1);
			return true;
		}
		return false;
	}
	else
	{
		m_currentIndex--;
		return true;
	}
}

bool Playlist::JumpToTrack(size_t index)
{
	if (index >= m_tracks.size())
		return false;
	m_currentIndex = index;
	return true;
}

size_t Playlist::Size() const
{
	return m_tracks.size();
}

bool Playlist::IsEmpty() const
{
	return m_tracks.empty();
}

size_t Playlist::GetCurrentIndex() const
{
	return m_currentIndex;
}

std::string Playlist::GetCurrentTrack() const
{
	if (m_tracks.empty())
		return "";
	return m_tracks[m_currentIndex];
}

std::vector<std::string> Playlist::GetTracks() const
{
	return m_tracks;
}

void Playlist::ToggleShuffle()
{
	m_shuffleEnabled = !m_shuffleEnabled;
	if (m_shuffleEnabled)
	{
		UpdateShuffleIndices();
	}
}

bool Playlist::IsShuffleEnabled() const
{
	return m_shuffleEnabled;
}

void Playlist::UpdateShuffleIndices()
{
	m_shuffleIndices.resize(m_tracks.size());
	std::iota(m_shuffleIndices.begin(), m_shuffleIndices.end(), 0);

	std::random_device rd;
	std::mt19937 gen(rd());

	// Keep current track at current position
	auto currentIt = std::find(m_shuffleIndices.begin(), m_shuffleIndices.end(), m_currentIndex);
	if (currentIt != m_shuffleIndices.end())
	{
		std::iter_swap(m_shuffleIndices.begin(), currentIt);
	}

	// Shuffle the rest
	if (m_shuffleIndices.size() > 1)
	{
		std::shuffle(m_shuffleIndices.begin() + 1, m_shuffleIndices.end(), gen);
	}
}
