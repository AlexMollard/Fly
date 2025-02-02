#include "pch.h"

#include "AudioVisualizer.h"

#define M_PI 3.14159265358979323846f

AudioVisualizer::AudioVisualizer()
      : ringBuffer(RING_BUFFER_SIZE), visualizerData(NUM_BANDS), bandPeaks(NUM_BANDS), bandDecay(NUM_BANDS), prevMagnitudes(NUM_BANDS), lastUpdateTime(std::chrono::steady_clock::now())
{
}

void AudioVisualizer::pushAudioData(const std::vector<float>& buffer, int channels, int sampleRate)
{
	std::lock_guard<std::mutex> lock(bufferMutex);
	currentSampleRate = sampleRate;

	// Convert to mono and add to ring buffer
	for (size_t i = 0; i < buffer.size(); i += channels)
	{
		float sample = 0.0f;
		for (int ch = 0; ch < channels; ch++)
		{
			// No need to divide by 32768 since data is already in float format
			sample += buffer[i + ch];
		}
		sample /= channels; // Average the channels

		sample = std::tanh(sample * 1.5f); // Soft limiting with slight amplification

		ringBuffer[writePos] = sample;
		writePos = (writePos + 1) & (RING_BUFFER_SIZE - 1); // Wrap around

		// If buffer is full, move read position
		if (writePos == readPos)
		{
			readPos = (readPos + 1) & (RING_BUFFER_SIZE - 1);
		}
	}
}

// Called from main/rendering thread
bool AudioVisualizer::update()
{
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();

	// Only update at specified interval
	if (elapsed < UPDATE_INTERVAL_MS)
	{
		return false;
	}

	lastUpdateTime = now;

	std::vector<float> processBuffer;
	{
		std::lock_guard<std::mutex> lock(bufferMutex);

		// Calculate available samples
		size_t available;
		if (writePos >= readPos)
		{
			available = writePos - readPos;
		}
		else
		{
			available = RING_BUFFER_SIZE - (readPos - writePos);
		}

		// Need enough samples for FFT
		if (available < FFT_SIZE)
		{
			return false;
		}

		// Copy samples for processing
		processBuffer.reserve(FFT_SIZE);
		for (size_t i = 0; i < FFT_SIZE; i++)
		{
			processBuffer.push_back(ringBuffer[(readPos + i) & (RING_BUFFER_SIZE - 1)]);
		}

		// Update read position
		readPos = (readPos + FFT_SIZE / 2) & (RING_BUFFER_SIZE - 1); // Overlap by 50%
	}

	// Process the audio data
	processFFT(processBuffer);
	return true;
}

void AudioVisualizer::processFFT(const std::vector<float>& samples)
{
	static const float RISE_SPEED = 0.7f;
	static const float FALL_SPEED = 0.15f;
	static const float PEAK_FALL_SPEED = 0.08f;
	static const float MIN_DB = -60.0f;
	static const float MAX_DB = -6.0f;

	// Scale samples to -1.0 to 1.0 range (knowing max is around 0.60)
	std::vector<float> normalizedSamples(samples.size());
	const float scaleAdjust = 0.05f; // Scale factor to normalize to -1.0 to 1.0
	for (size_t i = 0; i < samples.size(); i++)
	{
		normalizedSamples[i] = samples[i] * scaleAdjust;
	}

	// Perform FFT and get magnitudes
	std::vector<float> rawMagnitudes;
	performFFT(normalizedSamples, rawMagnitudes);

	std::vector<float> newMagnitudes(NUM_BANDS, 0.0f);
	float sampleRate = currentSampleRate > 0 ? currentSampleRate : 44100.0f;

	float minFreq = 20.0f;
	float maxFreq = 20000.0f;

	for (int band = 0; band < NUM_BANDS; band++)
	{
		float freq1 = minFreq * std::pow(maxFreq / minFreq, static_cast<float>(band) / NUM_BANDS);
		float freq2 = minFreq * std::pow(maxFreq / minFreq, static_cast<float>(band + 1) / NUM_BANDS);

		int bin1 = static_cast<int>(freq1 * FFT_SIZE / sampleRate);
		int bin2 = static_cast<int>(freq2 * FFT_SIZE / sampleRate);
		bin1 = std::clamp(bin1, 0, static_cast<int>(rawMagnitudes.size() - 1));
		bin2 = std::clamp(bin2, 0, static_cast<int>(rawMagnitudes.size() - 1));

		float sum = 0.0f;
		float weightSum = 0.0f;
		for (int bin = bin1; bin <= bin2; bin++)
		{
			float freq = bin * sampleRate / FFT_SIZE;
			// Very light frequency weighting
			float weight = 1.0f;
			if (freq < 100.0f)
				weight = 1.1f; // Slight bass boost
			if (freq > 10000.0f)
				weight = 1.05f; // Slight treble boost

			sum += rawMagnitudes[bin] * weight;
			weightSum += weight;
		}

		float avgMagnitude = (weightSum > 0.0f) ? (sum / weightSum) : 0.0f;

		// Dynamics processing
		float db = 20.0f * std::log10(avgMagnitude + 1e-6f);
		db = std::clamp(db, MIN_DB, MAX_DB);

		float normalizedMag = (db - MIN_DB) / (MAX_DB - MIN_DB);
		// Apply a moderate compression curve
		normalizedMag = std::pow(normalizedMag, 1.2f);
		// Scale to leave headroom
		normalizedMag *= 0.8f;

		// Temporal smoothing
		float delta = normalizedMag - prevMagnitudes[band];
		if (delta > 0)
		{
			newMagnitudes[band] = prevMagnitudes[band] + delta * RISE_SPEED;
		}
		else
		{
			newMagnitudes[band] = prevMagnitudes[band] + delta * FALL_SPEED;
		}

		// Peak tracking
		if (newMagnitudes[band] > bandPeaks[band])
		{
			bandPeaks[band] = newMagnitudes[band];
		}
		else
		{
			bandPeaks[band] *= (1.0f - PEAK_FALL_SPEED);
		}

		// Final safety clamp
		newMagnitudes[band] = std::clamp(newMagnitudes[band], 0.0f, 0.85f);
	}

	prevMagnitudes = newMagnitudes;
	visualizerData = newMagnitudes;
}

const std::vector<float>& AudioVisualizer::getVisualizerData() const
{
	return visualizerData;
}

const std::vector<float>& AudioVisualizer::getBandPeaks() const
{
	return bandPeaks;
}

void AudioVisualizer::performFFT(const std::vector<float>& samples, std::vector<float>& magnitudes)
{
	// Apply Hann window and prepare FFT input
	std::vector<std::complex<float>> fftInput(FFT_SIZE);
	for (size_t i = 0; i < min(samples.size(), size_t(FFT_SIZE)); i++)
	{
		float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (FFT_SIZE - 1)));
		fftInput[i] = std::complex<float>(samples[i] * window, 0.0f);
	}

	// Perform FFT (Cooley-Tukey algorithm)
	size_t n = fftInput.size();
	for (size_t k = 0; k < n; ++k)
	{
		size_t j = 0;
		for (size_t i = 0; i < n; ++i)
		{
			if (i < j)
			{
				std::swap(fftInput[i], fftInput[j]);
			}
			size_t m = n >> 1;
			while (j >= m && m > 0)
			{
				j -= m;
				m >>= 1;
			}
			j += m;
		}
	}

	for (size_t len = 2; len <= n; len <<= 1)
	{
		float angle = -2.0f * M_PI / static_cast<float>(len);
		std::complex<float> wlen(std::cos(angle), std::sin(angle));
		for (size_t i = 0; i < n; i += len)
		{
			std::complex<float> w(1.0f, 0.0f);
			for (size_t j = 0; j < len / 2; ++j)
			{
				std::complex<float> u = fftInput[i + j];
				std::complex<float> v = fftInput[i + j + len / 2] * w;
				fftInput[i + j] = u + v;
				fftInput[i + j + len / 2] = u - v;
				w *= wlen;
			}
		}
	}

	// Calculate magnitude spectrum
	magnitudes.resize(FFT_SIZE / 2);
	for (size_t i = 0; i < FFT_SIZE / 2; i++)
	{
		float mag = std::abs(fftInput[i]);
		magnitudes[i] = mag;
	}
}
