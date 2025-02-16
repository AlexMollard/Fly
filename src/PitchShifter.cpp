#include "pch.h"

#include "PitchShifter.h"

void PitchShifter::SetSource(ALuint source)
{
	m_source = source;
}

void PitchShifter::SetPitch(float semitones)
{
	m_pitchScale = std::pow(2.0f, semitones / 12.0f);
	if (m_source != 0)
	{
		alSourcef(m_source, AL_PITCH, m_pitchScale);
	}
}

float PitchShifter::GetPitch() const
{
	if (m_source != 0)
	{
		float pitch;
		alGetSourcef(m_source, AL_PITCH, &pitch);
		return std::log2(pitch) * 12.0f;
	}
	return std::log2(m_pitchScale) * 12.0f;
}

void PitchShifter::Reset()
{
	if (m_source != 0)
	{
		alSourcef(m_source, AL_PITCH, 1.0f);
	}
	m_pitchScale = 1.0f;
}
