#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>

#include "AudioVisualizer.h"

class MP3Player
{
private:
	ALCdevice* device;
	ALCcontext* context;
	ALuint source;
	ALuint buffer;
	std::vector<ALuint> buffers;

	bool isPlaying;
	float volume;
	float bass;
	float treble;
	std::string currentTrack;
	char filepath[256];
	double pausedTime = 0;

	std::vector<std::string> playlist;
	size_t currentTrackIndex;
	bool repeat;
	bool shuffle;

	ALuint lowpassFilter;  // For bass control
	ALuint highpassFilter; // For treble control
	ALuint pitchEffect;    // For pitch shifting
	float pitch;

	void initializeOpenAL();
	void cleanupOpenAL();
	void setupFilters();
	void updateFilters();

	ALuint effectSlots[2];
	ALuint effects[2];

	ALuint directFilters[2]; // Left and right filters

	std::vector<float> waveformData;

	static const int NUM_BUFFERS = 4;
	static const int BUFFER_SIZE = 16384; // 16KB chunks

	SNDFILE* streamFile;
	SF_INFO sfinfo;
	std::vector<ALuint> streamBuffers;
	bool streaming;

	sf_count_t totalFrames;    // Total number of frames in the audio file
	double streamDuration;     // Duration in seconds
	sf_count_t streamPosition; // Current position in seconds

	float positionX = 0.0f;
	float positionZ = 0.0f;
	float listenerX = 0.0f;
	float listenerZ = 0.0f;

	void updateStream();
	bool streamChunk(ALuint buffer);
	void clearStreamBuffers();

	AudioVisualizer visualizer;

public:
	MP3Player();
	~MP3Player();

	void setPosition(float x, float z);
	std::pair<float, float> getPosition() const;
	void setListenerPosition(float x, float z);
	std::pair<float, float> getListenerPosition() const;


	bool loadTrack(const std::string& filename);
	void playNext();
	void playPrevious();
	void clearPlaylist();
	void removeTrack(size_t index);

	void toggleRepeat();
	void toggleShuffle();
	bool isRepeatEnabled() const;
	bool isShuffleEnabled() const;

	void play();
	void pause();
	void stop();

	void setVolume(float newVolume);
	float getVolume() const;

	void setBass(float value);
	void setTreble(float value);
	float getBass() const;
	float getTreble() const;

	bool getIsPlaying() const;
	std::string getCurrentTrack() const;

	const std::vector<std::string>& getPlaylist() const;
	const char* getFilePath() const;
	void setFilePath(const char* path);

	void setPitch(float newPitch);
	float getPitch() const;

	std::vector<float> getWaveformData() const;

	void update();

	double getCurrentTime() const;
	void updateStreamPosition();
	double getDuration() const;
	void setCurrentTime(float percentage);
	void seekToPosition(double seconds);

	const std::vector<float>& getVisualizerData() const
	{
		return visualizer.getVisualizerData();
	}

	const std::vector<float>& getBandPeaks() const
	{
		return visualizer.getBandPeaks();
	}
};
