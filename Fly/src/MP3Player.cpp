#include "MP3Player.h"
#include <AL/efx.h>
#include <complex>
#include <cmath>


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

class AudioLimiter {
private:
    float threshold;
    float releaseTime;
    float currentGain;

public:
    AudioLimiter(float threshold = 0.9f, float releaseTime = 0.1f)
        : threshold(threshold), releaseTime(releaseTime), currentGain(1.0f) {}

    void process(std::vector<int16_t>& samples, int channels, int sampleRate) {
        float releaseCoeff = exp(-1.0f / (releaseTime * sampleRate));

        // Look ahead window for peak detection
        const int lookAhead = 32;
        std::vector<float> peaks(samples.size() / channels);

        // Find peaks
        for (size_t i = 0; i < samples.size(); i += channels) {
            float maxSample = 0.0f;
            for (int ch = 0; ch < channels; ch++) {
                float sample = std::abs(samples[i + ch] / 32768.0f);
                maxSample = std::max(maxSample, sample);
            }
            peaks[i / channels] = maxSample;
        }

        // Calculate gain reduction
        for (size_t i = 0; i < samples.size(); i += channels) {
            float maxPeak = 0.0f;
            // Look ahead for peaks
            for (int j = 0; j < lookAhead && (i / channels + j) < peaks.size(); j++) {
                maxPeak = std::max(maxPeak, peaks[i / channels + j]);
            }

            float targetGain = (maxPeak > threshold) ? threshold / maxPeak : 1.0f;
            currentGain = std::min(currentGain / releaseCoeff, targetGain);

            // Apply gain
            for (int ch = 0; ch < channels; ch++) {
                float sample = samples[i + ch] / 32768.0f;
                sample *= currentGain;
                samples[i + ch] = static_cast<int16_t>(std::clamp(sample * 32768.0f, -32768.0f, 32767.0f));
            }
        }
    }
};




MP3Player::MP3Player()
    : isPlaying(false), volume(40.0f), bass(100.0f), treble(0.0f),
    pitch(50.0f), currentTrackIndex(0), repeat(false), shuffle(false)
{
    initializeOpenAL();
    setupFilters();
}

MP3Player::~MP3Player()
{
    clearStreamBuffers();
    cleanupOpenAL();
}

void MP3Player::initializeOpenAL()
{
    // Open default device
    device = alcOpenDevice(nullptr);
    if (!device)
    {
        throw std::runtime_error("Failed to open OpenAL device");
    }

    ALCint refresh;
    alcGetIntegerv(device, ALC_REFRESH, 1, &refresh);

    // Set device parameters
    ALCint attrs[] = {
        ALC_REFRESH, refresh,
        ALC_SYNC, AL_FALSE,  // Disable forced synchronization
        ALC_MONO_SOURCES, 4,
        ALC_STEREO_SOURCES, 2,
        0
    };

    context = alcCreateContext(device, attrs);
    alcMakeContextCurrent(context);
    if (!context)
    {
        alcCloseDevice(device);
        throw std::runtime_error("Failed to create OpenAL context");
    }
    alcMakeContextCurrent(context);

    // Load EFX functions
    alDeleteFilters = (LPALDELETEFILTERS)alGetProcAddress("alDeleteFilters");
    if (!alDeleteFilters)
    {
        throw std::runtime_error("EFX not supported: Could not load alDeleteFilters");
    }

    alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
    if (!alDeleteEffects)
    {
        throw std::runtime_error("EFX not supported: Could not load alDeleteEffects");
    }

    alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
    if (!alDeleteAuxiliaryEffectSlots)
    {
        throw std::runtime_error("EFX not supported: Could not load alDeleteAuxiliaryEffectSlots");
    }

    alFilteri = (LPALFILTERI)alGetProcAddress("alFilteri");
    if (!alFilteri)
    {
        throw std::runtime_error("EFX not supported: Could not load alFilteri");
    }

    alFilterf = (LPALFILTERF)alGetProcAddress("alFilterf");
    if (!alFilterf)
    {
        throw std::runtime_error("EFX not supported: Could not load alFilterf");
    }

    alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
    if (!alEffecti)
    {
        throw std::runtime_error("EFX not supported: Could not load alEffecti");
    }

    alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
    if (!alEffectf)
    {
        throw std::runtime_error("EFX not supported: Could not load alEffectf");
    }

    alGenFilters = (LPALGENFILTERS)alGetProcAddress("alGenFilters");
    if (!alGenFilters)
    {
        throw std::runtime_error("EFX not supported: Could not load alGenFilters");
    }

    alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
    if (!alGenEffects)
    {
        throw std::runtime_error("EFX not supported: Could not load alGenEffects");
    }

    alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
    if (!alGenAuxiliaryEffectSlots)
    {
        throw std::runtime_error("EFX not supported: Could not load alGenAuxiliaryEffectSlots");
    }

    alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
    if (!alAuxiliaryEffectSloti)
    {
        throw std::runtime_error("EFX not supported: Could not load alAuxiliaryEffectSloti");
    }

    // Generate source
    alGenSources(1, &source);

    // Set initial source properties
    alSourcef(source, AL_PITCH, 1.0f);
    alSourcef(source, AL_GAIN, volume / 100.0f);
    alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcei(source, AL_LOOPING, AL_FALSE);
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

bool MP3Player::loadTrack(const std::string& filename) {
    stop();
    clearStreamBuffers();

    // Open audio file for streaming
    streamFile = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    if (!streamFile) {
        return false;
    }

    // Store total frames and calculate duration
    totalFrames = sfinfo.frames;
    streamDuration = static_cast<double>(totalFrames) / sfinfo.samplerate;
    streamPosition = 0.0;
    pausedTime = 0.0;

    // Generate streaming buffers
    streamBuffers.resize(NUM_BUFFERS);
    alGenBuffers(NUM_BUFFERS, streamBuffers.data());

    // Initial fill of all buffers
    for (ALuint buffer : streamBuffers) {
        if (!streamChunk(buffer)) {
            clearStreamBuffers();
            return false;
        }
        alSourceQueueBuffers(source, 1, &buffer);
    }

    currentTrack = filename;
    streaming = true;

    // Add to playlist if not already present
    if (std::find(playlist.begin(), playlist.end(), filename) == playlist.end()) {
        playlist.push_back(filename);
        currentTrackIndex = playlist.size() - 1;
    }

    return true;
}

void MP3Player::play() {
    if (!currentTrack.empty() && streaming) {
        if (pausedTime > 0.0) {
            seekToPosition(pausedTime);
        }
        alSourcePlay(source);
        isPlaying = true;
    }
}

void MP3Player::pause()
{
    if (isPlaying)
    {
        pausedTime = getCurrentTime();
        alSourcePause(source);
        isPlaying = false;
    }
}

void MP3Player::stop()
{
    if (!currentTrack.empty())
    {
        pausedTime = 0;
        alSourceStop(source);
        isPlaying = false;
    }
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

float MP3Player::getBass() const { return bass; }
float MP3Player::getTreble() const { return treble; }

const std::vector<std::string> &MP3Player::getPlaylist() const
{
    return playlist;
}

void MP3Player::cleanupOpenAL()
{
    alDeleteSources(1, &source);
    alDeleteBuffers(buffers.size(), buffers.data());
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
            } while (newIndex == currentTrackIndex);
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
            } while (newIndex == currentTrackIndex);
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

const char *MP3Player::getFilePath() const
{
    return filepath;
}

void MP3Player::setFilePath(const char *path)
{
    strncpy_s(filepath, path, sizeof(filepath) - 1);
    filepath[sizeof(filepath) - 1] = '\0';
}

void MP3Player::setPitch(float newPitch) {
    pitch = newPitch;
    updateFilters();
}

float MP3Player::getPitch() const {
    return pitch;
}

void MP3Player::update()
{
    if (isPlaying) {
        updateStream();
        visualizer.update();
    }
}

std::vector<float> MP3Player::getWaveformData() const {
    return waveformData;
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

void MP3Player::updateStream() {
    if (!streaming || !streamFile) return;

    ALint processed;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    while (processed--) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);

        if (!streamChunk(buffer)) {
            if (repeat) {
                sf_seek(streamFile, 0, SEEK_SET);
                streamChunk(buffer);
            }
        }
        alSourceQueueBuffers(source, 1, &buffer);
    }

    // Check playback status
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING && state != AL_PAUSED) {
        ALint queued;
        alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
        if (queued > 0) {
            alSourcePlay(source);
        }
    }
}

bool MP3Player::streamChunk(ALuint buffer) {
    std::vector<int16_t> data(BUFFER_SIZE * sfinfo.channels);

    sf_count_t samplesRead = sf_readf_short(streamFile, data.data(), BUFFER_SIZE);
    if (samplesRead > 0) {
        // Apply audio limiter
        AudioLimiter limiter(0.95f, 0.05f);
        limiter.process(data, sfinfo.channels, sfinfo.samplerate);

        // Process audio data for visualization before queuing
        visualizer.pushAudioData(data, sfinfo.channels, sfinfo.samplerate);

        // Update our stream position tracking
        updateStreamPosition();

        // Queue the processed audio data
        alBufferData(buffer,
            (sfinfo.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
            data.data(),
            samplesRead * sfinfo.channels * sizeof(int16_t),
            sfinfo.samplerate);

        return true;
    }
    return false;
}

void MP3Player::clearStreamBuffers() {
    if (!streamBuffers.empty()) {
        alSourceStop(source);
        alSourcei(source, AL_BUFFER, 0);
        alDeleteBuffers(streamBuffers.size(), streamBuffers.data());
        streamBuffers.clear();
    }

    if (streamFile) {
        sf_close(streamFile);
        streamFile = nullptr;
    }
}


double MP3Player::getDuration() const {
    if (!streamFile) return 0.0;
    return streamDuration;
}

double MP3Player::getCurrentTime() const {
    if (!streamFile) return 0.0;
    if (!isPlaying) return pausedTime;

    // Get the current position in the current buffer
    ALint sampleOffset;
    alGetSourcei(source, AL_SAMPLE_OFFSET, &sampleOffset);

    // Get number of processed buffers
    ALint processedBuffers;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processedBuffers);

    // Get current buffer position in samples
    // Each buffer is BUFFER_SIZE samples
    sf_count_t totalSamples = streamPosition + processedBuffers * BUFFER_SIZE + sampleOffset;

    return static_cast<double>(totalSamples) / sfinfo.samplerate;
}

void MP3Player::updateStreamPosition() {
    streamPosition += BUFFER_SIZE;
}

void MP3Player::seekToPosition(double seconds) {
    if (!streamFile) return;

    // Calculate frame position
    sf_count_t framePos = static_cast<sf_count_t>(seconds * sfinfo.samplerate);

    // Ensure position is within bounds
    framePos = std::clamp(framePos, static_cast<sf_count_t>(0), totalFrames);

    // Stop playback and clear existing buffers
    alSourceStop(source);

    // Unqueue all buffers
    ALint queued;
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    std::vector<ALuint> unqueuedBuffers(queued);
    if (queued > 0) {
        alSourceUnqueueBuffers(source, queued, unqueuedBuffers.data());
    }

    // Seek to new position
    sf_seek(streamFile, framePos, SEEK_SET);
    streamPosition = framePos;

    // Refill buffers from new position
    for (ALuint buffer : streamBuffers) {
        if (!streamChunk(buffer)) {
            break;
        }
        alSourceQueueBuffers(source, 1, &buffer);
    }

    // Resume playback if it was playing before
    if (isPlaying) {
        alSourcePlay(source);
    }

    pausedTime = seconds;
}

void MP3Player::setCurrentTime(float percentage) {
    if (!streamFile || percentage < 0.0f || percentage > 100.0f) return;

    double targetTime = (percentage / 100.0f) * streamDuration;
    seekToPosition(targetTime);
}