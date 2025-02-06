#pragma once
#include <AL/al.h>
#include <cmath>

class PitchShifter
{
private:
	float m_pitchScale = 1.0f;
	ALuint m_source = 0;

public:
	void SetSource(ALuint source);

	void SetPitch(float semitones);
	float GetPitch() const;

	void Reset();
};
