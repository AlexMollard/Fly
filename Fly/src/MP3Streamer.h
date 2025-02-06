#pragma once

#include <sndfile.h>
#include <string>
#include <vector>

#include "AudioStreamer.h"

class MP3Streamer : public AudioStreamer
{
public:
	MP3Streamer();
	~MP3Streamer();

	// Delete copy operations
	MP3Streamer(const MP3Streamer&) = delete;
	MP3Streamer& operator=(const MP3Streamer&) = delete;

	// Allow move operations
	MP3Streamer(MP3Streamer&& other) noexcept;
	MP3Streamer& operator=(MP3Streamer&& other) noexcept;

	// File operations
	bool openFromFile(const std::string& filename);
	void close();

	// Get Info
	const AudioStreamer::TrackInfo& GetTrackInfo();

protected:
	// AudioStreamer interface implementation
	bool onGetData(AudioChunk& chunk) override;
	void onSeek(double timeOffset) override;
	float onGetDuration() const override;
	std::optional<std::size_t> onLoop() override;

private:
	void cleanup();

	SNDFILE* m_file{ nullptr };
	SF_INFO m_fileInfo{};

	std::vector<float> m_sampleBuffer;
	static constexpr std::size_t BUFFER_SIZE = 16384; // Buffer size in samples (Might move into the cpp file)
};
