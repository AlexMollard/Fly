#pragma once

#include <sndfile.h>
#include <string>
#include <vector>

#include "AudioStreamer.h"
#include "AudioVisualizer.h"

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
	bool OpenFromFile(const std::string& filename);
	void Close();

	// Get Info
	const AudioStreamer::TrackInfo& GetTrackInfo();

	// Visualizer (Just calls into the member)
	const std::vector<float>& GetVisualizerData() const;
	const std::vector<float>& GetBandPeaks() const;

	void Update();

protected:
	// AudioStreamer interface implementation
	bool OnGetData(AudioChunk& chunk) override;
	void OnSeek(double timeOffset) override;
	float OnGetDuration() const override;
	std::optional<std::size_t> OnLoop() override;

private:
	void Cleanup();

	SNDFILE* m_file{ nullptr };
	SF_INFO m_fileInfo{};
	AudioVisualizer m_visualizer;

	std::vector<float> m_sampleBuffer;
};
