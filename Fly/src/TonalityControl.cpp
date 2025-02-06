#include "pch.h"

#include "TonalityControl.h"

// Set bass level (-1.0 to 1.0, where 0.0 is neutral)
void TonalityControl::SetBass(float level)
{
	m_inBass = level;
	m_bassGain = std::pow(20.0f, level); // Convert to gain
	m_bassGain = std::clamp(m_bassGain, 0.0f, 4.0f);
}

float TonalityControl::GetBass() const
{
	return m_inBass;
}

// Set treble level (-1.0 to 1.0, where 0.0 is neutral)
void TonalityControl::SetTreble(float level)
{
	m_inTreble = level;
	m_trebleGain = std::pow(20.0f, level); // Convert to gain
	m_trebleGain = std::clamp(m_trebleGain, 0.0f, 4.0f);
}

float TonalityControl::GetTreble() const
{
	return m_inTreble;
}

void TonalityControl::SetPitch(float level)
{
	m_pitchShifter.SetPitch(level);
}

float TonalityControl::GetPitch() const
{
	return m_pitchShifter.GetPitch();
}

std::function<void(std::vector<float>&, unsigned int, unsigned int)> TonalityControl::CreateProcessor()
{
	return [this](std::vector<float>& buffer, unsigned int channels, unsigned int sampleRate)
	{
		if (m_bassState.size() != channels)
		{
			m_bassState.resize(channels);
			m_trebleState.resize(channels);
		}

		// Lower bass frequency for deeper effect
		const float bassFreq = 80.0f; // Lowered from 100Hz to 80Hz
		const float bassQ = 0.5f;     // Lower Q for wider effect

		// Higher treble frequency for brighter effect
		const float trebleFreq = 12000.0f; // Raised from 10kHz to 12kHz
		const float trebleQ = 0.5f;        // Lower Q for wider effect

		auto [b0_bass, b1_bass, b2_bass, a1_bass, a2_bass] = CalculateShelfCoefficients(bassFreq, bassQ, m_bassGain, static_cast<float>(sampleRate), true);
		auto [b0_treble, b1_treble, b2_treble, a1_treble, a2_treble] = CalculateShelfCoefficients(trebleFreq, trebleQ, m_trebleGain, static_cast<float>(sampleRate), false);

		// Process EQ
		for (size_t i = 0; i < buffer.size(); i += channels)
		{
			for (unsigned int ch = 0; ch < channels; ch++)
			{
				float& sample = buffer[i + ch];
				float bass_out = ProcessSample(sample, b0_bass, b1_bass, b2_bass, a1_bass, a2_bass, m_bassState[ch]);
				sample = ProcessSample(bass_out, b0_treble, b1_treble, b2_treble, a1_treble, a2_treble, m_trebleState[ch]);
			}
		}
	};
}

// Calculate coefficients for shelf filter

std::tuple<float, float, float, float, float> TonalityControl::CalculateShelfCoefficients(float frequency, float Q, float gain, float sampleRate, bool isLowShelf)
{
	const float omega = 2.0f * M_PI * frequency / sampleRate;
	const float alpha = std::sin(omega) / (2.0f * Q);
	const float A = std::sqrt(gain);
	const float beta = std::sqrt(A) / Q;

	float b0, b1, b2, a0, a1, a2;

	if (isLowShelf)
	{
		// Lowshelf coefficients
		const float ap1 = A + 1.0f;
		const float am1 = A - 1.0f;
		const float cos_w = std::cos(omega);

		b0 = A * (ap1 - am1 * cos_w + beta * std::sin(omega));
		b1 = 2.0f * A * (am1 - ap1 * cos_w);
		b2 = A * (ap1 - am1 * cos_w - beta * std::sin(omega));
		a0 = ap1 + am1 * cos_w + beta * std::sin(omega);
		a1 = -2.0f * (am1 + ap1 * cos_w);
		a2 = ap1 + am1 * cos_w - beta * std::sin(omega);
	}
	else
	{
		// Highshelf coefficients
		const float ap1 = A + 1.0f;
		const float am1 = A - 1.0f;
		const float cos_w = std::cos(omega);

		b0 = A * (ap1 + am1 * cos_w + beta * std::sin(omega));
		b1 = -2.0f * A * (am1 + ap1 * cos_w);
		b2 = A * (ap1 + am1 * cos_w - beta * std::sin(omega));
		a0 = ap1 - am1 * cos_w + beta * std::sin(omega);
		a1 = 2.0f * (am1 - ap1 * cos_w);
		a2 = ap1 - am1 * cos_w - beta * std::sin(omega);
	}

	// Normalize coefficients
	const float norm = 1.0f / a0;
	return { b0 * norm, b1 * norm, b2 * norm, a1 * norm, a2 * norm };
}

// Process a single sample through the filter
float TonalityControl::ProcessSample(float input, float b0, float b1, float b2, float a1, float a2, FilterState& state)
{
	const float output = b0 * input + b1 * state.x1 + b2 * state.x2 - a1 * state.y1 - a2 * state.y2;

	// Update state
	state.x2 = state.x1;
	state.x1 = input;
	state.y2 = state.y1;
	state.y1 = output;

	return output;
}
