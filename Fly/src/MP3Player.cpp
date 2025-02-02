#include "pch.h"

#include "MP3Player.h"

#include <AL/alext.h>
#include <AL/efx.h>

static LPALDELETEFILTERS alDeleteFilters;
static LPALDELETEEFFECTS alDeleteEffects;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
static LPALFILTERI alFilteri;
static LPALFILTERF alFilterf;
static LPALEFFECTI alEffecti;
static LPALEFFECTF alEffectf;

static LPALGENFILTERS alGenFilters;
static LPALGENEFFECTS alGenEffects;
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;

static LPALCRESETDEVICESOFT alcResetDeviceSOFT;

void MP3Player::streamingThreadFunc()
{
	try
	{
		std::vector<float> readBuffer(BUFFER_SIZE * sfinfo.channels);
		std::vector<int16_t> stereoData(BUFFER_SIZE * 2);

		while (isRunning)
		{
			if (isPlaying && !isPaused && streamFile)
			{
				std::unique_lock<std::recursive_mutex> lock(stateMutex);

				// Get current buffer state
				ALint processed = 0;
				alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

				// Calculate minimum required buffers
				const int minRequiredBuffers = 2; // Minimum safety margin
				int targetBuffers = NUM_BUFFERS;

				// Adjust target based on playback state
				if (processed > targetBuffers / 2)
					targetBuffers = max(minRequiredBuffers, targetBuffers - 1);

				ALint queued = 0;
				alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

				// Check for errors
				ALenum error = alGetError();
				if (error != AL_NO_ERROR)
				{
					LOG_ERROR("Error getting source state: {}", error);
					continue;
				}

				LOG_DEBUG("Streaming thread: processed = {}, queued = {}", processed, queued);

				// First unqueue any processed buffers
				if (processed > 0)
				{
					std::vector<ALuint> unqueuedBuffers(processed);
					alSourceUnqueueBuffers(source, processed, unqueuedBuffers.data());
					error = alGetError();
					if (error != AL_NO_ERROR)
					{
						LOG_ERROR("Error unqueueing buffers: {}", error);
						continue;
					}
					LOG_DEBUG("Unqueued {} buffers", processed);
				}

				// Fill and queue new buffers until we have targetBuffers queued
				int retryCount = 0;
				while (queued < targetBuffers && currentFrame < totalFrames)
				{
					ALuint buffer;
					alGenBuffers(1, &buffer);
					streamBuffers.push_back(buffer);

					sf_count_t framesRead = sf_readf_float(streamFile, readBuffer.data(), BUFFER_SIZE);
					if (framesRead <= 0)
					{
						if (repeat && currentFrame >= totalFrames)
						{
							sf_seek(streamFile, 0, SEEK_SET);
							currentFrame = 0;
							continue;
						}

						// Add retry logic
						if (++retryCount < MAX_RETRIES)
						{
							sf_seek(streamFile, currentFrame - BUFFER_SIZE, SEEK_SET);
							continue;
						}

						LOG_WARN("End of stream reached after {} retries due to framesRead being 0!", retryCount);
						isPlaying = false;
						break;
					}

					// Convert and queue buffer
					convertToStereo(readBuffer, stereoData, framesRead);
					alBufferData(buffer, AL_FORMAT_STEREO16, stereoData.data(), static_cast<ALsizei>(framesRead * 2 * sizeof(int16_t)), sfinfo.samplerate);

					error = alGetError();
					if (error != AL_NO_ERROR)
					{
						LOG_ERROR("Error buffering audio data: {}", error);
						continue;
					}

					alSourceQueueBuffers(source, 1, &buffer);
					error = alGetError();
					if (error != AL_NO_ERROR)
					{
						LOG_ERROR("Error queueing buffer: {}", error);
						continue;
					}

					currentFrame += framesRead;
					queued++;

					visualizer.pushAudioData(readBuffer, sfinfo.channels, sfinfo.samplerate);
					LOG_DEBUG("Queued buffer: framesRead = {}, currentFrame = {}", framesRead, currentFrame.load());
				}

				// Start playback if needed
				ALint state;
				alGetSourcei(source, AL_SOURCE_STATE, &state);
				if (state != AL_PLAYING && isPlaying && !isPaused)
				{
					alSourcePlay(source);
					error = alGetError();
					if (error != AL_NO_ERROR)
					{
						LOG_ERROR("Error starting playback: {}", error);
					}
					else
					{
						LOG_DEBUG("Playback started");
					}
				}

				lock.unlock();
			}
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Streaming thread error: {}", e.what());
	}
}

void MP3Player::visualizerThreadFunc()
{
	while (isRunning)
	{
		if (isPlaying)
		{
			AudioBuffer buffer;
			if (audioQueue.pop(buffer))
			{
				// Process visualization data in a separate thread
				visualizer.pushAudioData(buffer.data, static_cast<int>(buffer.size), sfinfo.channels);
			}
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
}

void MP3Player::convertToStereo(const std::vector<float>& input, std::vector<int16_t>& output, size_t frames)
{
	for (size_t i = 0; i < frames; i++)
	{
		for (int ch = 0; ch < min(sfinfo.channels, 2); ch++)
		{
			float sample = input[i * sfinfo.channels + ch];
			sample = std::tanh(sample);
			output[i * 2 + ch] = static_cast<int16_t>(std::clamp(sample * 32767.0f, -32767.0f, 32767.0f));
		}
	}
}

MP3Player::MP3Player()
      : isPlaying(false), volume(40.0f), bass(100.0f), treble(0.0f), pitch(50.0f), currentTrackIndex(0), repeat(false), shuffle(false)
{
	try
	{
		initializeOpenAL();
		setupFilters();

		isRunning = true;
		streamingThread = std::thread(&MP3Player::streamingThreadFunc, this);
	}
	catch (const std::exception& e)
	{
		cleanup();
		throw std::runtime_error(std::string("MP3Player initialization failed: ") + e.what());
	}
}

MP3Player::~MP3Player()
{
	cleanup();
}

void MP3Player::cleanup()
{
	// Stop all threads
	isRunning = false;
	isPaused = false;
	isPlaying = false;
	audioQueue.terminate();

	if (streamingThread.joinable())
	{
		streamingThread.join();
	}

	// Clean up OpenAL resources
	std::lock_guard<std::mutex> lock(deviceMutex);

	clearStreamBuffers();

	if (context)
	{
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		context = nullptr;
	}

	if (device)
	{
		alcCloseDevice(device);
		device = nullptr;
	}
}

void MP3Player::initializeOpenAL()
{
	// Open default device
	device = alcOpenDevice(nullptr);
	if (!device)
	{
		throw std::runtime_error("Failed to open OpenAL device");
	}

	// Get device refresh rate
	ALCint refresh;
	alcGetIntegerv(device, ALC_REFRESH, 1, &refresh);

	// Set device parameters
	ALCint attrs[] = { ALC_FREQUENCY,
		48000,
		ALC_REFRESH,
		refresh,
		ALC_SYNC,
		AL_TRUE,
		ALC_MONO_SOURCES,
		4,
		ALC_STEREO_SOURCES,
		2,
		ALC_MAX_AUXILIARY_SENDS,
		4,
		// Force stereo output
		ALC_STEREO_SOURCES,
		2,
		ALC_OUTPUT_MODE_SOFT,
		ALC_STEREO_HRTF_SOFT, // Add this
		ALC_HRTF_SOFT,
		ALC_TRUE,
		0 };

	context = alcCreateContext(device, attrs);
	if (!context)
	{
		alcCloseDevice(device);
		throw std::runtime_error("Failed to create OpenAL context");
	}
	alcMakeContextCurrent(context);

	// Check for float format support
	if (!alIsExtensionPresent("AL_EXT_FLOAT32"))
	{
		throw std::runtime_error("OpenAL float32 format not supported");
	}

	// Load EFX functions
	alcResetDeviceSOFT = (LPALCRESETDEVICESOFT) alcGetProcAddress(nullptr, "alcResetDeviceSOFT");
	if (!alcResetDeviceSOFT)
	{
		throw std::runtime_error("ALC_SOFT_reset_device not supported");
	}

	alDeleteFilters = (LPALDELETEFILTERS) alGetProcAddress("alDeleteFilters");
	if (!alDeleteFilters)
	{
		throw std::runtime_error("EFX not supported: Could not load alDeleteFilters");
	}

	alDeleteEffects = (LPALDELETEEFFECTS) alGetProcAddress("alDeleteEffects");
	if (!alDeleteEffects)
	{
		throw std::runtime_error("EFX not supported: Could not load alDeleteEffects");
	}

	alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS) alGetProcAddress("alDeleteAuxiliaryEffectSlots");
	if (!alDeleteAuxiliaryEffectSlots)
	{
		throw std::runtime_error("EFX not supported: Could not load alDeleteAuxiliaryEffectSlots");
	}

	alFilteri = (LPALFILTERI) alGetProcAddress("alFilteri");
	if (!alFilteri)
	{
		throw std::runtime_error("EFX not supported: Could not load alFilteri");
	}

	alFilterf = (LPALFILTERF) alGetProcAddress("alFilterf");
	if (!alFilterf)
	{
		throw std::runtime_error("EFX not supported: Could not load alFilterf");
	}

	alEffecti = (LPALEFFECTI) alGetProcAddress("alEffecti");
	if (!alEffecti)
	{
		throw std::runtime_error("EFX not supported: Could not load alEffecti");
	}

	alEffectf = (LPALEFFECTF) alGetProcAddress("alEffectf");
	if (!alEffectf)
	{
		throw std::runtime_error("EFX not supported: Could not load alEffectf");
	}

	alGenFilters = (LPALGENFILTERS) alGetProcAddress("alGenFilters");
	if (!alGenFilters)
	{
		throw std::runtime_error("EFX not supported: Could not load alGenFilters");
	}

	alGenEffects = (LPALGENEFFECTS) alGetProcAddress("alGenEffects");
	if (!alGenEffects)
	{
		throw std::runtime_error("EFX not supported: Could not load alGenEffects");
	}

	alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS) alGetProcAddress("alGenAuxiliaryEffectSlots");
	if (!alGenAuxiliaryEffectSlots)
	{
		throw std::runtime_error("EFX not supported: Could not load alGenAuxiliaryEffectSlots");
	}

	alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI) alGetProcAddress("alAuxiliaryEffectSloti");
	if (!alAuxiliaryEffectSloti)
	{
		throw std::runtime_error("EFX not supported: Could not load alAuxiliaryEffectSloti");
	}

	alGenSources(1, &source);

	ALCint hrtf_status;
	alcGetIntegerv(device, ALC_HRTF_STATUS_SOFT, 1, &hrtf_status);

	if (hrtf_status == ALC_HRTF_ENABLED_SOFT)
	{
		const ALchar* hrtf_name = alcGetString(device, ALC_HRTF_SPECIFIER_SOFT);
		LOG_INFO("HRTF is enabled using: {}", hrtf_name ? hrtf_name : "unknown");
	}
	else
	{
		LOG_WARN("HRTF is not enabled");
	}

	// Set initial source properties with error checking
	alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);       // World-space positioning
	alSourcei(source, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE); // Enable spatization
	alSourcef(source, AL_CONE_INNER_ANGLE, 360.0f);
	alSourcef(source, AL_CONE_OUTER_ANGLE, 360.0f);

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

	// Check for errors
	ALenum error = alGetError();
	if (error != AL_NO_ERROR)
	{
		throw std::runtime_error("Error initializing OpenAL source");
	}
}

void MP3Player::setupFilters()
{
	// Generate filters
	alGenFilters(1, &lowpassFilter);
	alGenFilters(1, &highpassFilter);

	// Set filter types
	alFilteri(lowpassFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
	alFilteri(highpassFilter, AL_FILTER_TYPE, AL_FILTER_HIGHPASS);

	// Create effect for pitch shifting
	alGenEffects(1, &pitchEffect);
	alEffecti(pitchEffect, AL_EFFECT_TYPE, AL_EFFECT_PITCH_SHIFTER);

	// Create auxiliary effect slot for pitch
	alGenAuxiliaryEffectSlots(1, effectSlots);
	alAuxiliaryEffectSloti(effectSlots[0], AL_EFFECTSLOT_EFFECT, pitchEffect);

	// Initial filter values
	updateFilters();
}

void MP3Player::updateFilters()
{
	// Bass control (lowpass filter)
	// Map bass value (0-100) to frequency range (50Hz - 2000Hz)
	float bassFreq = 50.0f + (bass / 100.0f) * 1950.0f;
	alFilterf(lowpassFilter, AL_LOWPASS_GAIN, 1.0f);
	alFilterf(lowpassFilter, AL_LOWPASS_GAINHF, bass / 100.0f);

	// Treble control (highpass filter)
	// Map treble value (0-100) to frequency range (2000Hz - 20000Hz)
	float trebleFreq = 2000.0f + (treble / 100.0f) * 18000.0f;
	alFilterf(highpassFilter, AL_HIGHPASS_GAIN, 1.0f);
	alFilterf(highpassFilter, AL_HIGHPASS_GAINLF, treble / 100.0f);

	// Apply filters to source
	alSourcei(source, AL_DIRECT_FILTER, lowpassFilter);
	alSource3i(source, AL_AUXILIARY_SEND_FILTER, effectSlots[0], 0, highpassFilter);

	// Update pitch directly on the source
	// Map pitch from 0-100 to 0.75-1.25 range for more subtle pitch adjustment
	// 50 maps to 1.0 (normal pitch)
	float pitchValue = 1.0f + ((pitch - 50.0f) / 100.0f) * 0.5f;
	alSourcef(source, AL_PITCH, pitchValue);
}

bool MP3Player::loadTrack(const std::string& filename)
{
	try
	{
		std::lock_guard<std::recursive_mutex> lock(stateMutex);

		stop();
		clearStreamBuffers();

		streamFile = sf_open(filename.c_str(), SFM_READ, &sfinfo);
		if (!streamFile)
		{
			LOG_ERROR("Failed to open audio file: {}", filename);
			throw std::runtime_error("Failed to open audio file");
		}

		// Check file format and log details
		LOG_DEBUG("Opened file: {}, Format: {}, Channels: {}, Samplerate: {}, Frames: {}",
			filename, sfinfo.format, sfinfo.channels, sfinfo.samplerate, sfinfo.frames);

		// Initialize streaming buffers
		streamBuffers.resize(NUM_BUFFERS);
		alGenBuffers(NUM_BUFFERS, streamBuffers.data());

		// Reset counters
		totalFrames = sfinfo.frames;
		streamDuration = static_cast<double>(totalFrames) / sfinfo.samplerate;
		currentFrame = 0;
		pausedTime = 0.0;

		LOG_DEBUG("Loading track: {}, Frames: {}, Duration: {:.2f}s", filename, totalFrames, streamDuration.load());

		// Initial buffer fill
		std::vector<float> initialData(BUFFER_SIZE * sfinfo.channels);
		std::vector<int16_t> stereoData(BUFFER_SIZE * 2);

		// Fill all initial buffers
		for (ALuint buffer: streamBuffers)
		{
			sf_count_t framesRead = sf_readf_float(streamFile, initialData.data(), BUFFER_SIZE);
			LOG_DEBUG("Initial frames read: {}", framesRead);

			if (framesRead > 0)
			{
				convertToStereo(initialData, stereoData, framesRead);

				alBufferData(buffer, AL_FORMAT_STEREO16, stereoData.data(), static_cast<ALsizei>(framesRead * 2 * sizeof(int16_t)), sfinfo.samplerate);

				alSourceQueueBuffers(source, 1, &buffer);

				currentFrame += framesRead;
				LOG_DEBUG("Initial buffer filled: {} frames, position: {}", framesRead, currentFrame.load());
			}
		}

		currentTrack = filename;

		// Update playlist
		if (std::find(playlist.begin(), playlist.end(), filename) == playlist.end())
		{
			playlist.push_back(filename);
			currentTrackIndex = playlist.size() - 1;
		}
		else
		{
			auto it = std::find(playlist.begin(), playlist.end(), filename);
			currentTrackIndex = std::distance(playlist.begin(), it);
		}

		// Set source properties
		alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
		alSourcei(source, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE);
		alSourcef(source, AL_GAIN, volume / 100.0f);

		float position[3] = { positionX, 0.0f, positionZ };
		alSourcefv(source, AL_POSITION, position);

		LOG_DEBUG("Track loaded successfully");
		return true;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Failed to load track: {}", e.what());
		clearStreamBuffers();
		return false;
	}
}

void MP3Player::play()
{
	if (currentTrack.empty())
		return;

	try
	{
		std::lock_guard<std::recursive_mutex> lock(stateMutex);

		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		LOG_DEBUG("Current source state before play: {}", state);

		ALint queued;
		alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
		LOG_DEBUG("Queued buffers before play: {}", queued);

		if (queued == 0)
		{
			LOG_DEBUG("No buffers queued, reloading track");
			if (!loadTrack(currentTrack))
			{
				return;
			}
		}

		alSourcePlay(source);
		ALenum error = alGetError();
		if (error != AL_NO_ERROR)
		{
			LOG_ERROR("Error playing source: {}", error);
			return;
		}

		alGetSourcei(source, AL_SOURCE_STATE, &state);
		LOG_DEBUG("Source state after play: {}", state);

		isPaused = false;
		isPlaying = true;

		// Additional debug logs
		alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
		LOG_DEBUG("Queued buffers after play: {}", queued);
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Play error: {}", e.what());
	}
}

void MP3Player::pause()
{
	if (!isPlaying)
		return;

	{
		std::lock_guard<std::recursive_mutex> lock(stateMutex);
		alSourcePause(source);
	}

	isPaused = true;
	isPlaying = false;
}

void MP3Player::stop()
{
	{
		std::lock_guard<std::recursive_mutex> lock(stateMutex);
		alSourceStop(source);
	}

	isPlaying = false;
	isPaused = false;
	audioQueue.clear();
}

void MP3Player::setVolume(float newVolume)
{
	volume = newVolume;
	alSourcef(source, AL_GAIN, volume / 100.0f);
}

void MP3Player::setBass(float value)
{
	bass = value;
	updateFilters();
}

void MP3Player::setTreble(float value)
{
	treble = value;
	updateFilters();
}

float MP3Player::getBass() const
{
	return bass;
}

float MP3Player::getTreble() const
{
	return treble;
}

const std::vector<std::string>& MP3Player::getPlaylist() const
{
	return playlist;
}

void MP3Player::cleanupOpenAL()
{
	alDeleteSources(1, &source);
	alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
	alDeleteFilters(1, &lowpassFilter);
	alDeleteFilters(1, &highpassFilter);
	alDeleteEffects(1, &pitchEffect);
	alDeleteAuxiliaryEffectSlots(1, effectSlots);

	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

void MP3Player::playNext()
{
	if (playlist.empty())
		return;

	if (shuffle)
	{
		// Generate random index excluding current track
		size_t newIndex;
		if (playlist.size() > 1)
		{
			do
			{
				newIndex = rand() % playlist.size();
			}
			while (newIndex == currentTrackIndex);
			currentTrackIndex = newIndex;
		}
	}
	else
	{
		currentTrackIndex = (currentTrackIndex + 1) % playlist.size();
		if (!repeat && currentTrackIndex == 0)
		{
			stop();
			return;
		}
	}

	loadTrack(playlist[currentTrackIndex]);
	play();
}

void MP3Player::playPrevious()
{
	if (playlist.empty())
		return;

	if (shuffle)
	{
		// In shuffle mode, generate random previous track
		size_t newIndex;
		if (playlist.size() > 1)
		{
			do
			{
				newIndex = rand() % playlist.size();
			}
			while (newIndex == currentTrackIndex);
			currentTrackIndex = newIndex;
		}
	}
	else
	{
		if (currentTrackIndex == 0)
		{
			if (repeat)
			{
				currentTrackIndex = playlist.size() - 1;
			}
			else
			{
				return;
			}
		}
		else
		{
			currentTrackIndex--;
		}
	}

	loadTrack(playlist[currentTrackIndex]);
	play();
}

void MP3Player::clearPlaylist()
{
	stop();
	playlist.clear();
	currentTrackIndex = 0;
	currentTrack.clear();
}

void MP3Player::toggleRepeat()
{
	repeat = !repeat;
}

void MP3Player::toggleShuffle()
{
	shuffle = !shuffle;
	if (shuffle)
	{
		// Seed the random number generator when enabling shuffle
		srand(static_cast<unsigned int>(time(nullptr)));
	}
}

bool MP3Player::isRepeatEnabled() const
{
	return repeat;
}

bool MP3Player::isShuffleEnabled() const
{
	return shuffle;
}

float MP3Player::getVolume() const
{
	return volume;
}

bool MP3Player::getIsPlaying() const
{
	return isPlaying;
}

std::string MP3Player::getCurrentTrack() const
{
	return currentTrack;
}

const char* MP3Player::getFilePath() const
{
	return filepath;
}

void MP3Player::setFilePath(const char* path)
{
	strncpy_s(filepath, path, sizeof(filepath) - 1);
	filepath[sizeof(filepath) - 1] = '\0';
}

void MP3Player::setPitch(float newPitch)
{
	pitch = newPitch;
	updateFilters();
}

float MP3Player::getPitch() const
{
	return pitch;
}

void MP3Player::update()
{
	if (!isPlaying)
		return;

	try
	{
		// Only check and update the playing state
		ALint state;
		alGetSourcei(source, AL_SOURCE_STATE, &state);

		if (state != AL_PLAYING && isPlaying && !isPaused)
		{
			ALint queued;
			alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

			if (queued > 0)
			{
				alSourcePlay(source);
				LOG_DEBUG("Restarted playback in update() with {} buffers", queued);
			}
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Update error: {}", e.what());
	}
}

void MP3Player::removeTrack(size_t index)
{
	if (index < playlist.size())
	{
		// If removing currently playing track, stop playback
		if (playlist[index] == currentTrack)
		{
			stop();
			currentTrack.clear();
		}
		playlist.erase(playlist.begin() + index);
	}
}

void MP3Player::clearStreamBuffers()
{
	std::lock_guard<std::recursive_mutex> lock(stateMutex);

	if (!streamBuffers.empty())
	{
		alSourceStop(source);
		alSourcei(source, AL_BUFFER, 0);
		alDeleteBuffers(static_cast<ALsizei>(streamBuffers.size()), streamBuffers.data());
		streamBuffers.clear();
	}

	if (streamFile)
	{
		sf_close(streamFile);
		streamFile = nullptr;
	}
}

double MP3Player::getDuration() const
{
	if (!streamFile)
		return 0.0;
	return streamDuration;
}

double MP3Player::getCurrentTime() const
{
	if (!streamFile)
		return 0.0;
	if (isPaused)
		return pausedTime;
	if (!isPlaying)
		return 0.0;

	try
	{
		// Get current playback position
		ALint sampleOffset;
		alGetSourcei(source, AL_SAMPLE_OFFSET, &sampleOffset);

		// Get number of processed buffers
		ALint processedBuffers;
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &processedBuffers);

		// Calculate total samples played
		sf_count_t totalSamples = streamPosition + (static_cast<sf_count_t>(processedBuffers) * BUFFER_SIZE) + sampleOffset;

		return static_cast<double>(totalSamples) / sfinfo.samplerate;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("getCurrentTime error: {}", e.what());
		return pausedTime;
	}
}

void MP3Player::updateStreamPosition()
{
	streamPosition += BUFFER_SIZE;
}

void MP3Player::seekToPosition(double seconds)
{
	if (!streamFile)
		return;

	try
	{
		std::lock_guard<std::recursive_mutex> lock(stateMutex);

		sf_count_t framePos = static_cast<sf_count_t>(seconds * sfinfo.samplerate);
		framePos = std::clamp(framePos, static_cast<sf_count_t>(0), totalFrames);

		// Clear existing audio queue
		audioQueue.clear();

		// Stop and clear OpenAL buffers
		alSourceStop(source);

		ALint queued;
		alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
		if (queued > 0)
		{
			std::vector<ALuint> unqueuedBuffers(queued);
			alSourceUnqueueBuffers(source, queued, unqueuedBuffers.data());
		}

		// Seek in the file
		sf_seek(streamFile, framePos, SEEK_SET);
		currentFrame = framePos; // Update the frame counter
		pausedTime = seconds;

		// If we were playing, restart playback
		if (isPlaying)
		{
			play();
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Seek error: {}", e.what());
	}
}

void MP3Player::setCurrentTime(float percentage)
{
	if (!streamFile || percentage < 0.0f || percentage > 100.0f)
		return;

	double targetTime = (percentage / 100.0f) * streamDuration;
	seekToPosition(targetTime);
}

void MP3Player::setPosition(float x, float z)
{
	positionX = x;
	positionZ = z;

	// Create a proper position vector (x, y, z)
	float position[3] = { x, 0.0f, z }; // y = 0 for 2D positioning
	alSourcefv(source, AL_POSITION, position);

	alSourcef(source, AL_MAX_DISTANCE, 1000.0f);
	alSourcef(source, AL_REFERENCE_DISTANCE, 1.0f);
	alSourcef(source, AL_ROLLOFF_FACTOR, 1.0f);

	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
}

void MP3Player::setListenerPosition(float x, float z)
{
	listenerX = x;
	listenerZ = z;

	// Create proper position vector
	float position[3] = { x, 0.0f, z };
	alListenerfv(AL_POSITION, position);

	// Forward vector (-z) and Up vector (+y) for orientation
	float orientation[6] = {
		0.0f,
		0.0f,
		-1.0f, // Forward vector
		0.0f,
		1.0f,
		0.0f // Up vector
	};
	alListenerfv(AL_ORIENTATION, orientation);
}

std::pair<float, float> MP3Player::getListenerPosition() const
{
	return { listenerX, listenerZ };
}

std::pair<float, float> MP3Player::getPosition() const
{
	return { positionX, positionZ };
}
