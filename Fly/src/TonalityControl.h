#pragma once
#include "PitchShifter.h"

class TonalityControl
{
private:
	// Filter coefficients and state
	struct FilterState
	{
		float x1{ 0 }, x2{ 0 }; // Input history
		float y1{ 0 }, y2{ 0 }; // Output history
	};

	float m_bassGain{ 1.0f };
	float m_trebleGain{ 1.0f };
	float m_inBass{ 0.0f };   // Range: -1.0 to 1.0
	float m_inTreble{ 0.0f }; // Range: -1.0 to 1.0

	std::vector<FilterState> m_bassState;
	std::vector<FilterState> m_trebleState;

	ALuint m_source{ 0 };
	PitchShifter m_pitchShifter;

public:
	void SetSource(ALuint source)
	{
		m_source = source;
		m_pitchShifter.SetSource(source);
	}

	// Set bass level (-1.0 to 1.0, where 0.0 is neutral)
	void SetBass(float level);
	float GetBass() const;

	// Set treble level (-1.0 to 1.0, where 0.0 is neutral)
	void SetTreble(float level);
	float GetTreble() const;

	// Set pitch level (-1.0 to 1.0, where 0.0 is neutral)
	void SetPitch(float level);
	float GetPitch() const;

	// This will make it into a lambda function for the audio streamer
	std::function<void(std::vector<float>&, unsigned int, unsigned int)> CreateProcessor();

private:
	// Calculate coefficients for shelf filter
	std::tuple<float, float, float, float, float> CalculateShelfCoefficients(float frequency, float Q, float gain, float sampleRate, bool isLowShelf);

	// Process a single sample through the filter
	float ProcessSample(float input, float b0, float b1, float b2, float a1, float a2, FilterState& state);
};
