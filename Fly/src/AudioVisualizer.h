#pragma once

class AudioVisualizer
{
private:
	static constexpr size_t RING_BUFFER_SIZE = 16384; // Power of 2 for efficient wrapping
	static constexpr size_t UPDATE_INTERVAL_MS = 16;  // ~60 FPS
	static const int NUM_BANDS = 23;                  // Number of frequency bands
	static const int FFT_SIZE = 2048;                 // Size of FFT window

	std::vector<float> m_ringBuffer;
	size_t m_writePos = 0;
	size_t m_readPos = 0;

	std::vector<float> m_visualizerData;
	std::vector<float> m_bandPeaks;
	std::vector<float> m_bandDecay;
	std::vector<float> m_prevMagnitudes;

	std::chrono::steady_clock::time_point m_lastUpdateTime;

	// Mutex for thread safety
	std::mutex m_bufferMutex;

	int m_currentSampleRate;

	void PerformFFT(const std::vector<float>& samples, std::vector<float>& magnitudes);

public:
	AudioVisualizer();

	// Called from audio streaming thread
	void PushAudioData(const std::vector<float>& buffer, int channels, int sampleRate);

	// Called from main/rendering thread
	bool Update();
	void ProcessFFT(const std::vector<float>& samples);

	const std::vector<float>& GetVisualizerData() const;
	const std::vector<float>& GetBandPeaks() const;
};
