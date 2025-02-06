#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "containers/ThreadSafeQueue.h"

class AudioStreamer
{
public:
	struct TrackInfo
	{
		std::string title;
		std::string artist;
		std::string album;
		std::string genre;
		std::string year;
		float duration{ 0.0f };
	};

	enum class Status
	{
		Stopped,
		Playing,
		Paused
	};

	struct StreamingConfig
	{
		unsigned int channelCount{ 2 };
		unsigned int sampleRate{ 44100 };
		unsigned int bufferSize{ 16384 }; // 16KB chunks
		unsigned int numBuffers{ 4 };
	};

	struct AudioChunk
	{
		const float* samples{ nullptr };
		std::size_t sampleCount{ 0 };
	};

	struct AudioBuffer
	{
		std::vector<float> data;
		std::size_t size{ 0 };
		bool isLast{ false };
	};

	using EffectProcessor = std::function<void(std::vector<float>&, unsigned int, unsigned int)>;

	AudioStreamer();
	~AudioStreamer();

	// Prevent copying
	AudioStreamer(const AudioStreamer&) = delete;
	AudioStreamer& operator=(const AudioStreamer&) = delete;

	// Allow moving
	AudioStreamer(AudioStreamer&& other) noexcept;
	AudioStreamer& operator=(AudioStreamer&& other) noexcept;

	// Core functionality
	void initialize(const StreamingConfig& config);
	void createAndFillBuffers(bool recreateBuffers);
	void play();
	void pause();
	void stop(bool clearInfo = false);
	void cleanup();

	// State control
	void setLooping(bool loop);

	bool isLooping() const
	{
		return m_looping;
	}

	Status getStatus() const
	{
		return m_status;
	}

	// Audio properties
	void setVolume(float newVolume);

	float getVolume() const
	{
		return m_volume;
	}

	unsigned int getChannelCount() const
	{
		return m_config.channelCount;
	}

	unsigned int getSampleRate() const
	{
		return m_config.sampleRate;
	}

	// Position control
	void setPlayingOffset(double timeOffset);
	double getPlayingOffset() const;
	float getDuration() const;

	// Effects
	void setEffectProcessor(EffectProcessor processor)
	{
		m_effectProcessor = processor;
	}

	void clearEffectProcessor()
	{
		m_effectProcessor = nullptr;
	}

protected:
	// Virtual methods for derived classes
	virtual bool onGetData(AudioChunk& chunk) = 0;
	virtual void onSeek(double timeOffset) = 0;
	virtual std::optional<std::size_t> onLoop();

	// Returns total number of samples in the stream
	virtual float onGetDuration() const = 0;

	// Track info
	AudioStreamer::TrackInfo m_trackInfo{};

private:
	void initializeOpenAL();
	void cleanupOpenAL();
	void streamingThreadFunc();
	void updateBufferStream();
	void checkAlError(const char* operation);

	// OpenAL state
	ALCdevice* m_device{ nullptr };
	ALCcontext* m_context{ nullptr };
	ALuint m_source{ 0 };
	std::vector<ALuint> m_buffers;

	// Streaming state
	StreamingConfig m_config;
	std::atomic<Status> m_status{ Status::Stopped };
	std::atomic<bool> m_isRunning{ false };
	std::atomic<bool> m_looping{ false };
	std::atomic<float> m_volume{ 0.5f };
	std::atomic<std::size_t> m_samplesProcessed{ 0 };

	// Thread management
	std::thread m_streamingThread;
	ThreadSafeQueue<AudioBuffer> m_audioQueue;
	std::recursive_mutex m_streamMutex;

	// Audio processing
	std::vector<float> m_processingBuffer;
	EffectProcessor m_effectProcessor;
};
