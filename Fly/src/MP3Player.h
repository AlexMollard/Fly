#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <queue>
#include <sndfile.h>

#include "AudioVisualizer.h"

class MP3Player
{
private:
	// Thread-safe audio buffer queue
	struct AudioBuffer
	{
		std::vector<float> data;
		size_t size;
		bool isLast;
	};

	class ThreadSafeQueue
	{
	private:
		std::queue<AudioBuffer> queue;
		mutable std::mutex mutex;
		std::condition_variable cond;
		std::atomic<bool> terminated{ false };

	public:
		size_t size()
		{
			return queue.size();
		}

		void push(AudioBuffer&& buffer)
		{
			{
				std::lock_guard<std::mutex> lock(mutex);
				queue.push(std::move(buffer));
			}
			cond.notify_one();
		}

		bool pop(AudioBuffer& buffer)
		{
			std::unique_lock<std::mutex> lock(mutex);
			while (queue.empty() && !terminated)
			{
				cond.wait(lock);
			}

			if (terminated && queue.empty())
			{
				return false;
			}

			buffer = std::move(queue.front());
			queue.pop();
			return true;
		}

		void clear()
		{
			std::lock_guard<std::mutex> lock(mutex);
			std::queue<AudioBuffer> empty;
			std::swap(queue, empty);
		}

		void terminate()
		{
			{
				std::lock_guard<std::mutex> lock(mutex);
				terminated = true;
			}
			cond.notify_all();
		}
	};

	mutable std::recursive_mutex stateMutex;

	// Constants
	static const int BUFFER_SIZE = 16384; // 16KB chunks
	static const int NUM_BUFFERS = 4;
	static const int MAX_RETRIES = 4;

	// OpenAL state
	ALCdevice* device;
	ALCcontext* context;
	ALuint source;
	std::vector<ALuint> buffers;

	// Track management
	size_t currentTrackIndex = 0;
	bool repeat = false;
	bool shuffle = false;
	std::vector<std::string> playlist;

	// Thread control
	std::atomic<bool> isRunning{ false };
	std::atomic<bool> isPaused{ false };
	std::thread streamingThread;
	std::thread visualizerThread;
	ThreadSafeQueue audioQueue;
	std::mutex deviceMutex;

	// Playback state
	std::string currentTrack;
	std::atomic<double> pausedTime{ 0 };
	std::atomic<bool> isPlaying{ false };
	std::atomic<bool> streaming{ false };
	std::atomic<sf_count_t> currentFrame{ 0 };
	std::atomic<float> volume{ 40.0f };
	std::atomic<float> bass{ 100.0f };
	std::atomic<float> treble{ 0.0f };
	std::atomic<float> pitch{ 50.0f };

	// File streaming state
	SNDFILE* streamFile;
	SF_INFO sfinfo;
	sf_count_t totalFrames;
	std::atomic<sf_count_t> streamPosition{ 0 };
	std::atomic<double> streamDuration{ 0.0 };
	std::vector<ALuint> streamBuffers;

	// Filters and effects
	ALuint lowpassFilter;  // For bass control
	ALuint highpassFilter; // For treble control
	ALuint pitchEffect;    // For pitch shifting
	ALuint effectSlots[2];
	ALuint effects[2];

	// Spatial audio
	std::atomic<float> positionX{ 0.0f };
	std::atomic<float> positionZ{ 0.0f };
	std::atomic<float> listenerX{ 0.0f };
	std::atomic<float> listenerZ{ 0.0f };

	char filepath[256];

	void streamingThreadFunc();
	void visualizerThreadFunc();

	void convertToStereo(const std::vector<float>& input, std::vector<int16_t>& output, size_t frames);

	void initializeOpenAL();
	void cleanupOpenAL();
	void setupFilters();
	void updateFilters();
	void clearStreamBuffers();

	AudioVisualizer visualizer;

public:
	MP3Player();
	~MP3Player();

	void cleanup();

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
