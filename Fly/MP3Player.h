#pragma once

#include <SFML/Audio.hpp>
#include <iostream>

class MP3Player {
private:
	sf::Music music;
	std::string currentTrack;
	bool isPlaying;
	float volume;
	sf::Time duration;
	char filepath[256] = "";
	std::vector<std::string> playlist;
	size_t currentTrackIndex;
	bool repeat;
	bool shuffle;

	float bass;
	float treble;
	sf::SoundStream::Status previousStatus;

public:
	MP3Player();

	bool loadTrack(const std::string& filename);

	void playNext();
	void playPrevious();

	void clearPlaylist();

	void toggleRepeat();
	void toggleShuffle();
	bool isRepeatEnabled() const;
	bool isShuffleEnabled() const;

	void play();
	void pause();
	void stop();

	void setBass(float value);
	void setTreble(float value);
	float getBass() const;
	float getTreble() const;

	void setVolume(float newVolume);
	float getVolume() const;

	bool getIsPlaying() const;
	std::string getCurrentTrack() const;
	sf::Time getDuration() const;
	sf::Time getCurrentTime() const;
	const std::vector<std::string>& getPlaylist() const;

	void setCurrentTime(float percentage);

	const char* getFilePath() const;
	void setFilePath(const char* path);
};