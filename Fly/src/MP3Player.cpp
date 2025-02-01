#include "MP3Player.h"
#include <AL/efx.h>
#include <sndfile.h>

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

MP3Player::MP3Player()
    : isPlaying(false), volume(80.0f), bass(50.0f), treble(50.0f), currentTrackIndex(0), repeat(false), shuffle(false)
{
    initializeOpenAL();
    setupFilters();
}

MP3Player::~MP3Player()
{
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

    // Create context
    context = alcCreateContext(device, nullptr);
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
    // Generate effect slot
    alGenAuxiliaryEffectSlots(1, &effectSlot);

    // Create equalizer effect
    alGenEffects(1, &equalizerEffect);
    alEffecti(equalizerEffect, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);

    // Create filters
    alGenFilters(1, &lowpassFilter);
    alGenFilters(1, &highpassFilter);

    alFilteri(lowpassFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
    alFilteri(highpassFilter, AL_FILTER_TYPE, AL_FILTER_HIGHPASS);

    updateFilters();
}

void MP3Player::updateFilters()
{
    // Update low-pass filter (bass)
    float bassGain = bass / 50.0f; // Convert 0-100 range to 0-2 range
    alFilterf(lowpassFilter, AL_LOWPASS_GAIN, bassGain);
    alFilterf(lowpassFilter, AL_LOWPASS_GAINHF, 0.5f + (bass / 200.0f));

    // Update high-pass filter (treble)
    float trebleGain = treble / 50.0f;
    alFilterf(highpassFilter, AL_HIGHPASS_GAIN, trebleGain);
    alFilterf(highpassFilter, AL_HIGHPASS_GAINLF, 0.5f + (treble / 200.0f));

    // Apply filters to source
    alSource3i(source, AL_AUXILIARY_SEND_FILTER, effectSlot, 0, AL_FILTER_NULL);
    alSourcei(source, AL_DIRECT_FILTER, lowpassFilter);
}

bool MP3Player::loadTrack(const std::string &filename)
{
    stop();

    if (!loadAudioFile(filename))
    {
        return false;
    }

    currentTrack = filename;

    // Add to playlist if not already present
    if (std::find(playlist.begin(), playlist.end(), filename) == playlist.end())
    {
        playlist.push_back(filename);
        currentTrackIndex = playlist.size() - 1;
    }

    return true;
}

void MP3Player::play()
{
    if (!currentTrack.empty())
    {
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
    alDeleteEffects(1, &equalizerEffect);
    alDeleteAuxiliaryEffectSlots(1, &effectSlot);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

bool MP3Player::loadAudioFile(const std::string &filename)
{
    // libsndfile we need to decode the audio file into raw PCM data
    SF_INFO sfinfo;
    SNDFILE *file = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    if (!file)
    {
        return false;
    }

    // Read audio data
    const int bufferSize = 4096;
    std::vector<int16_t> pcmData(bufferSize * sfinfo.channels);
    std::vector<int16_t> allData;
    sf_count_t bytesRead = 0;
    while ((bytesRead = sf_readf_short(file, pcmData.data(), bufferSize)) > 0)
    {
        allData.insert(allData.end(), pcmData.begin(), pcmData.begin() + bytesRead * sfinfo.channels);
    }
    sf_close(file);

    // Convert to raw PCM data
    const int dataSize = allData.size() * sizeof(int16_t);

    // Generate buffer
    ALuint buffer;
    alGenBuffers(1, &buffer);
    buffers.push_back(buffer);

    // Upload audio data to buffer
    alBufferData(buffer, (sfinfo.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, allData.data(), dataSize, sfinfo.samplerate);

    // Attach buffer to source
    alSourcei(source, AL_BUFFER, buffer);

    return true;
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

double MP3Player::getDuration() const
{
    ALint sizeInBytes;
    ALint channels;
    ALint bits;
    ALint frequency;

    if (buffers.empty())
        return 0.0;

    alGetBufferi(buffers.back(), AL_SIZE, &sizeInBytes);
    alGetBufferi(buffers.back(), AL_CHANNELS, &channels);
    alGetBufferi(buffers.back(), AL_BITS, &bits);
    alGetBufferi(buffers.back(), AL_FREQUENCY, &frequency);

    if (frequency == 0 || channels == 0 || bits == 0)
        return 0.0;

    // Calculate duration in seconds
    double duration = static_cast<double>(sizeInBytes * 8) / (channels * bits * frequency);
    return duration;
}

double MP3Player::getCurrentTime() const
{
    if (!isPlaying)
        return pausedTime;

    ALfloat seconds = 0.0f;
    alGetSourcef(source, AL_SEC_OFFSET, &seconds);
    return static_cast<double>(seconds);
}

void MP3Player::setCurrentTime(float percentage)
{
    if (percentage < 0.0f || percentage > 100.0f)
        return;

    ALfloat duration = static_cast<ALfloat>(getDuration());
    ALfloat newPosition = duration * (percentage / 100.0f);
    alSourcef(source, AL_SEC_OFFSET, newPosition);
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