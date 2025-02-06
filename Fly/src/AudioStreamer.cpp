#include "pch.h"

#include "AudioStreamer.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <AL/alext.h>

AudioStreamer::AudioStreamer()
{
	LOG_DEBUG("Initializing AudioStreamer");
	try
	{
		InitOpenAL();
		LOG_INFO("AudioStreamer successfully initialized");
	}
	catch ([[maybe_unused]] const std::exception& e)
	{
		LOG_ERROR("Failed to initialize AudioStreamer: {}", e.what());
		Cleanup();
		throw;
	}
}

AudioStreamer::~AudioStreamer()
{
	LOG_DEBUG("Destroying AudioStreamer");
	Cleanup();
}

AudioStreamer::AudioStreamer(AudioStreamer&& other) noexcept
      : m_device(std::exchange(other.m_device, nullptr)),
        m_context(std::exchange(other.m_context, nullptr)),
        m_source(std::exchange(other.m_source, 0)),
        m_buffers(std::move(other.m_buffers)),
        m_config(std::move(other.m_config)),
        m_status(other.m_status.load()),
        m_isRunning(other.m_isRunning.load()),
        m_looping(other.m_looping.load()),
        m_volume(other.m_volume.load()),
        m_samplesProcessed(other.m_samplesProcessed.load()),
        m_streamingThread(std::move(other.m_streamingThread)),
        m_audioQueue(std::move(other.m_audioQueue)),
        m_processingBuffer(std::move(other.m_processingBuffer)),
        m_effectProcessor(std::move(other.m_effectProcessor))
{
}

AudioStreamer& AudioStreamer::operator=(AudioStreamer&& other) noexcept
{
	if (this != &other)
	{
		Cleanup();

		m_device = std::exchange(other.m_device, nullptr);
		m_context = std::exchange(other.m_context, nullptr);
		m_source = std::exchange(other.m_source, 0);
		m_buffers = std::move(other.m_buffers);
		m_config = std::move(other.m_config);
		m_status = other.m_status.load();
		m_isRunning = other.m_isRunning.load();
		m_looping = other.m_looping.load();
		m_volume = other.m_volume.load();
		m_samplesProcessed = other.m_samplesProcessed.load();
		m_streamingThread = std::move(other.m_streamingThread);
		m_audioQueue = std::move(other.m_audioQueue);
		m_processingBuffer = std::move(other.m_processingBuffer);
		m_effectProcessor = std::move(other.m_effectProcessor);
	}

	return *this;
}

void AudioStreamer::InitOpenAL()
{
	LOG_DEBUG("Initializing OpenAL");

	// Open default device
	m_device = alcOpenDevice(nullptr);
	if (!m_device)
	{
		LOG_ERROR("Failed to open OpenAL device");
		throw std::runtime_error("Failed to open OpenAL device");
	}
	LOG_INFO("OpenAL device opened successfully");

	// Get device refresh rate
	ALCint refresh;
	alcGetIntegerv(m_device, ALC_REFRESH, 1, &refresh);

	// Create context with high-quality settings
	std::array<ALCint, 15> attrs = { ALC_FREQUENCY,
		48000,
		ALC_REFRESH,
		refresh,
		ALC_SYNC,
		AL_TRUE,
		ALC_MONO_SOURCES,
		4,
		ALC_STEREO_SOURCES,
		2,
		ALC_OUTPUT_MODE_SOFT,
		ALC_STEREO_HRTF_SOFT,
		ALC_HRTF_SOFT,
		ALC_TRUE,
		0 };

	m_context = alcCreateContext(m_device, attrs.data());
	if (!m_context)
	{
		LOG_ERROR("Failed to create OpenAL context");
		alcCloseDevice(m_device);
		throw std::runtime_error("Failed to create OpenAL context");
	}
	LOG_INFO("OpenAL context created with frequency: {}, refresh: {}", attrs[1], attrs[3]);

	alcMakeContextCurrent(m_context);

	// Check for float format support
	if (!alIsExtensionPresent("AL_EXT_FLOAT32"))
	{
		LOG_WARN("OpenAL implementation does not support 32-bit float format");
	}
	else
	{
		LOG_DEBUG("OpenAL implementation supports 32-bit float format");
	}

	// Generate source
	alGenSources(1, &m_source);
	CheckAlError("Failed to generate source");
	LOG_DEBUG("OpenAL source generated successfully");

	// Configure source properties
	alSourcef(m_source, AL_PITCH, 1.0f);
	alSourcef(m_source, AL_GAIN, m_volume);
	alSourcei(m_source, AL_SOURCE_RELATIVE, AL_TRUE);
	LOG_DEBUG("Source properties configured - Pitch: 1.0, Initial volume: {}", m_volume.load());

		ALCint hrtf_status;
	alcGetIntegerv(m_device, ALC_HRTF_STATUS_SOFT, 1, &hrtf_status);

	if (hrtf_status == ALC_HRTF_ENABLED_SOFT)
	{
		const ALchar* hrtf_name = alcGetString(m_device, ALC_HRTF_SPECIFIER_SOFT);
		LOG_INFO("HRTF is enabled using: {}", hrtf_name ? hrtf_name : "unknown");
	}
	else
	{
		LOG_WARN("HRTF is not enabled");
	}

	// Set initial source properties with error checking
	alSourcei(m_source, AL_SOURCE_RELATIVE, AL_FALSE);     // World-space positioning
	alSourcei(m_source, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE); // Enable spatization
	alSourcef(m_source, AL_CONE_INNER_ANGLE, 360.0f);
	alSourcef(m_source, AL_CONE_OUTER_ANGLE, 360.0f);

	alSpeedOfSound(343.3f); // Speed of sound in m/s
	alDopplerFactor(1.0f);  // Realistic doppler effect
	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

	// Initialize listener position and orientation
	float listenerPos[3] = { 0.0f, 0.0f, 0.0f };
	float listenerOri[6] = { 0.0f,
		0.0f,
		-1.0f, // Forward vector
		0.0f,
		1.0f,
		0.0f }; // Up vector
	alListenerfv(AL_POSITION, listenerPos);
	alListenerfv(AL_ORIENTATION, listenerOri);
	alListenerf(AL_GAIN, 1.0f);

	// Start streaming thread
	m_isRunning = true;
	m_streamingThread = std::thread(&AudioStreamer::StreamingThreadFunc, this);
	SetThreadDescription(m_streamingThread.native_handle(), L"AudioStreamer");
	LOG_INFO("Streaming thread started");
}

void AudioStreamer::Cleanup()
{
	LOG_DEBUG("Starting AudioStreamer cleanup");
	m_isRunning = false;
	m_audioQueue.terminate();

	if (m_streamingThread.joinable())
	{
		LOG_DEBUG("Joining streaming thread");
		m_streamingThread.join();
	}

	CleanupOpenAL();
	LOG_INFO("AudioStreamer cleanup completed");
}

void AudioStreamer::CleanupOpenAL()
{
	LOG_DEBUG("Starting OpenAL cleanup");

	if (m_source)
	{
		alSourceStop(m_source);
		alDeleteSources(1, &m_source);
		LOG_DEBUG("OpenAL source deleted");
	}

	if (!m_buffers.empty())
	{
		LOG_DEBUG("Deleting {} OpenAL buffers", m_buffers.size());
		alDeleteBuffers(static_cast<ALsizei>(m_buffers.size()), m_buffers.data());
		m_buffers.clear();
	}

	if (m_context)
	{
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(m_context);
		m_context = nullptr;
		LOG_DEBUG("OpenAL context destroyed");
	}

	if (m_device)
	{
		alcCloseDevice(m_device);
		m_device = nullptr;
		LOG_DEBUG("OpenAL device closed");
	}
}

void AudioStreamer::StreamingThreadFunc()
{
	LOG_DEBUG("Streaming thread started");
	while (m_isRunning)
	{
		if (m_status == Status::Playing)
		{
			std::lock_guard<std::recursive_mutex> lock(m_streamMutex);
			UpdateBufferStream();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	LOG_DEBUG("Streaming thread stopped");
}

void AudioStreamer::UpdateBufferStream()
{
	ALint processed = 0;
	alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);

	while (processed--)
	{
		ALuint buffer;
		alSourceUnqueueBuffers(m_source, 1, &buffer);
		CheckAlError("Failed to unqueue buffer");

		AudioChunk chunk;
		bool gotData = OnGetData(chunk);

		if (gotData && chunk.samples && chunk.sampleCount > 0)
		{
			// Convert float samples to int16_t
			std::vector<int16_t> convertedBuffer(chunk.sampleCount);
			for (size_t i = 0; i < chunk.sampleCount; ++i)
			{
				// Convert float (-1.0 to 1.0) to int16_t (-32768 to 32767)
				float sample = std::clamp(chunk.samples[i], -1.0f, 1.0f);
				convertedBuffer[i] = static_cast<int16_t>(sample * 32767.0f);
			}

			if (m_effectProcessor)
			{
				m_processingBuffer.resize(chunk.sampleCount);
				std::memcpy(m_processingBuffer.data(), chunk.samples, chunk.sampleCount * sizeof(float));
				m_effectProcessor(m_processingBuffer, m_config.channelCount, m_config.sampleRate);

				// Convert processed float samples to int16_t
				for (size_t i = 0; i < chunk.sampleCount; ++i)
				{
					float sample = std::clamp(m_processingBuffer[i], -1.0f, 1.0f);
					convertedBuffer[i] = static_cast<int16_t>(sample * 32767.0f);
				}
			}

			alBufferData(buffer, m_config.channelCount == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, convertedBuffer.data(), static_cast<ALsizei>(chunk.sampleCount * sizeof(int16_t)), m_config.sampleRate);

			CheckAlError("Failed to buffer audio data");
			alSourceQueueBuffers(m_source, 1, &buffer);
			CheckAlError("Failed to queue buffer");

			m_samplesProcessed += chunk.sampleCount;
		}
		else
		{
			LOG_WARN("No audio data received for buffer");
		}
	}

	ALint state;
	alGetSourcei(m_source, AL_SOURCE_STATE, &state);

	if (state != AL_PLAYING && m_status == Status::Playing)
	{
		ALint queued;
		alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
		if (queued > 0)
		{
			LOG_INFO("Restarting playback with {} buffers queued", queued);
			alSourcePlay(m_source);
			CheckAlError("Failed to restart playback");
		}
	}
}

void AudioStreamer::Play()
{
	if (m_status != Status::Playing)
	{
		ALint queued;
		alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
		if (queued > 0)
		{
			LOG_INFO("Starting playback with {} buffers queued", queued);
			alSourcePlay(m_source);
			CheckAlError("Failed to start playback");
			m_status = Status::Playing;
		}
		else
		{
			LOG_WARN("Cannot start playback - no buffers queued");
		}
	}
}

void AudioStreamer::Pause()
{
	if (m_status == Status::Playing)
	{
		LOG_INFO("Pausing playback");
		alSourcePause(m_source);
		CheckAlError("Failed to pause playback");
		m_status = Status::Paused;
	}
}

void AudioStreamer::Stop(bool clearInfo)
{
	LOG_INFO("Stopping playback");
	alSourceStop(m_source);
	CheckAlError("Failed to stop playback");
	m_status = Status::Stopped;
	m_samplesProcessed = 0;
	
	if (clearInfo)
	{
		// Reset m_trackInfo
		m_trackInfo = {};
	}
}

void AudioStreamer::setVolume(float newVolume)
{
	m_volume = std::clamp(newVolume, 0.0f, 1.0f);
	alSourcef(m_source, AL_GAIN, m_volume);
	LOG_DEBUG("Volume set to {}", m_volume.load());
}

void AudioStreamer::SetLooping(bool shouldLoop)
{
	m_looping = shouldLoop;
	alSourcei(m_source, AL_LOOPING, shouldLoop ? AL_TRUE : AL_FALSE);
	LOG_INFO("Looping {} for audio source", shouldLoop ? "enabled" : "disabled");
}

std::optional<std::size_t> AudioStreamer::OnLoop()
{
	SetPlayingOffset(0.0);

	return 0;
}

float AudioStreamer::GetDuration() const
{
	if (m_config.sampleRate == 0)
		return 0.0;

	return OnGetDuration();
}

double AudioStreamer::GetPlayingOffset() const
{
	ALint sampleOffset;

	alGetSourcei(m_source, AL_SAMPLE_OFFSET, &sampleOffset);
	return static_cast<double>(m_samplesProcessed + sampleOffset) / (m_config.sampleRate * m_config.channelCount);
}

void AudioStreamer::SetPosition(float x, float z)
{
	m_positionX = std::clamp(x, -1.0f, 1.0f);
	m_positionZ = std::clamp(z, -1.0f, 1.0f);

	alSource3f(m_source, AL_POSITION, m_positionX, 0.0f, m_positionZ);
	CheckAlError("Failed to set source position");
}

void AudioStreamer::SetListenerPosition(float x, float z)
{
	m_listenerX = std::clamp(x, -1.0f, 1.0f);
	m_listenerZ = std::clamp(z, -1.0f, 1.0f);

	alListener3f(AL_POSITION, m_listenerX, 0.0f, m_listenerZ);
	CheckAlError("Failed to set listener position");
}

void AudioStreamer::SetPlayingOffset(double timeOffset)
{
	if (m_config.sampleRate == 0)
	{
		LOG_WARN("Cannot set playing offset: sample rate is 0");
		return;
	}

	LOG_INFO("Setting playing offset to {} seconds", timeOffset);
	std::lock_guard<std::recursive_mutex> lock(m_streamMutex);

	Stop();

	std::size_t frame = static_cast<std::size_t>(timeOffset * m_config.sampleRate);
	m_samplesProcessed = frame * m_config.channelCount;

	OnSeek(timeOffset);

	CreateAndFillBuffers(false); // false = don't recreate existing buffers

	Play();
}

void AudioStreamer::CheckAlError(const char* operation)
{
	ALenum error = alGetError();
	if (error != AL_NO_ERROR)
	{
		LOG_ERROR("{}: OpenAL error {}", operation, error);
		throw std::runtime_error(std::string(operation) + ": OpenAL error " + std::to_string(error));
	}
}

void AudioStreamer::Init(const StreamingConfig& newConfig)
{
	LOG_INFO("Initializing stream with {} channels, {} Hz sample rate, {} buffers", newConfig.channelCount, newConfig.sampleRate, newConfig.numBuffers);
	m_samplesProcessed = 0;
	m_config = newConfig;

	Stop(true); // false = clear track info as a new track is being loaded

	CreateAndFillBuffers(false); // false = don't recreate if buffers exist

	m_status = Status::Playing;
	Play();
}

void AudioStreamer::CreateAndFillBuffers(bool recreateBuffers)
{
	if (recreateBuffers && !m_buffers.empty())
	{
		LOG_DEBUG("Clearing existing buffers");
		alDeleteBuffers(static_cast<ALsizei>(m_buffers.size()), m_buffers.data());
		m_buffers.clear();
	}

	if (m_buffers.empty())
	{
		LOG_DEBUG("Generating {} OpenAL buffers", m_config.numBuffers);
		m_buffers.resize(m_config.numBuffers);
		alGenBuffers(m_config.numBuffers, m_buffers.data());
		CheckAlError("Failed to generate buffers");
	}
	else
	{
		// Clear existing queued buffers
		ALint queued;
		alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
		if (queued > 0)
		{
			LOG_DEBUG("Clearing {} queued buffers", queued);
			alSourcei(m_source, AL_BUFFER, 0);
			CheckAlError("Failed to clear queued buffers");
		}
	}

	// Fill buffers with initial audio data
	for (ALuint buffer: m_buffers)
	{
		AudioChunk chunk;
		bool gotData = OnGetData(chunk);

		if (gotData && chunk.samples && chunk.sampleCount > 0)
		{
			LOG_DEBUG("Queueing initial buffer with {} samples", chunk.sampleCount);

			// Convert float samples to int16_t
			std::vector<int16_t> convertedBuffer(chunk.sampleCount);
			for (size_t i = 0; i < chunk.sampleCount; ++i)
			{
				// Convert float (-1.0 to 1.0) to int16_t (-32768 to 32767)
				float sample = std::clamp(chunk.samples[i], -1.0f, 1.0f);
				convertedBuffer[i] = static_cast<int16_t>(sample * 32767.0f);
			}


			alBufferData(buffer, m_config.channelCount == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, convertedBuffer.data(), static_cast<ALsizei>(chunk.sampleCount * sizeof(int16_t)), m_config.sampleRate);
			CheckAlError("Failed to fill initial buffer");

			alSourceQueueBuffers(m_source, 1, &buffer);
			CheckAlError("Failed to queue initial buffer");

			m_samplesProcessed += chunk.sampleCount;
		}
		else
		{
			LOG_WARN("Failed to get initial audio data");
			break;
		}
	}
}
