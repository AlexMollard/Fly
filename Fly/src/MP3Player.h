#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>

class MP3Player
{
private:
    ALCdevice *device;
    ALCcontext *context;
    ALuint source;
    ALuint buffer;
    std::vector<ALuint> buffers; // For streaming implementation

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

    // OpenAL Effect objects
    ALuint effectSlot;
    ALuint equalizerEffect;
    ALuint lowpassFilter;  // For bass control
    ALuint highpassFilter; // For treble control

    void initializeOpenAL();
    void cleanupOpenAL();
    void setupFilters();
    bool loadAudioFile(const std::string &filename);
    void updateFilters();

public:
    MP3Player();
    ~MP3Player();

    bool loadTrack(const std::string &filename);
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

    void setVolume(float newVolume);
    float getVolume() const;

    void setBass(float value);
    void setTreble(float value);
    float getBass() const;
    float getTreble() const;

    bool getIsPlaying() const;
    std::string getCurrentTrack() const;

    // Get duration and current time in seconds
    double getDuration() const;
    double getCurrentTime() const;
    void setCurrentTime(float percentage);

    const std::vector<std::string> &getPlaylist() const;
    const char *getFilePath() const;
    void setFilePath(const char *path);
};