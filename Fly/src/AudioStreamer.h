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
	void Init(const StreamingConfig& config);
	void CreateAndFillBuffers(bool recreateBuffers);
	void Play();
	void Pause();
	void Stop(bool clearInfo = false);
	void Cleanup();

	// State control
	void SetLooping(bool loop);

	bool IsLooping() const
	{
		return m_looping;
	}

	Status GetStatus() const
	{
		return m_status;
	}

	// Audio properties
	void setVolume(float newVolume);

	float GetVolume() const
	{
		return m_volume;
	}

	unsigned int GetChannelCount() const
	{
		return m_config.channelCount;
	}

	unsigned int GetSampleRate() const
	{
		return m_config.sampleRate;
	}

	// Position control
	void SetPlayingOffset(double timeOffset);
	double GetPlayingOffset() const;
	float GetDuration() const;

	// Effects
	void SetEffectProcessor(EffectProcessor processor)
	{
		m_effectProcessor = processor;
	}

	void ClearEffectProcessor()
	{
		m_effectProcessor = nullptr;
	}

protected:
	// Virtual methods for derived classes
	virtual bool OnGetData(AudioChunk& chunk) = 0;
	virtual void OnSeek(double timeOffset) = 0;
	virtual std::optional<std::size_t> OnLoop();

	// Returns total number of samples in the stream
	virtual float OnGetDuration() const = 0;

	// Track info
	AudioStreamer::TrackInfo m_trackInfo{};

private:
	void InitOpenAL();
	void CleanupOpenAL();
	void StreamingThreadFunc();
	void UpdateBufferStream();
	void CheckAlError(const char* operation);

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
