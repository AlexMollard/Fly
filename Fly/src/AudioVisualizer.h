#pragma once

class AudioVisualizer
{
private:
	static constexpr size_t RING_BUFFER_SIZE = 16384; // Power of 2 for efficient wrapping
	static constexpr size_t UPDATE_INTERVAL_MS = 16;  // ~60 FPS

	std::vector<float> ringBuffer;
	size_t writePos = 0;
	size_t readPos = 0;

	std::vector<float> visualizerData;
	std::vector<float> bandPeaks;
	std::vector<float> bandDecay;
	std::vector<float> prevMagnitudes;

	std::chrono::steady_clock::time_point lastUpdateTime;

	// Mutex for thread safety
	std::mutex bufferMutex;

	static const int NUM_BANDS = 32;  // Number of frequency bands
	static const int FFT_SIZE = 2048; // Size of FFT window

	int currentSampleRate;

	void performFFT(const std::vector<float>& samples, std::vector<float>& magnitudes);

public:
	AudioVisualizer();

	// Called from audio streaming thread
	void pushAudioData(const std::vector<int16_t>& buffer, int channels, int sampleRate);

	// Called from main/rendering thread
	bool update();
	void processFFT(const std::vector<float>& samples);

	const std::vector<float>& getVisualizerData() const;
	const std::vector<float>& getBandPeaks() const;
};
