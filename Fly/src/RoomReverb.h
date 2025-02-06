#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>

class RoomReverb
{
private:
	ALuint m_effect = 0;
	ALuint m_slot = 0;
	ALCdevice* m_device = nullptr;

	// Current reverb parameters
	float m_decayTime = 1.0f;         // 0.1 to 20.0 seconds
	float m_reflectionsDelay = 0.02f; // 0.0 to 0.3 seconds
	float m_lateDelay = 0.03f;        // 0.0 to 0.1 seconds
	float m_roomRolloff = 0.0f;       // 0.0 to 10.0
	float m_decayHFRatio = 1.0f;      // 0.1 to 2.0
	float m_reflectionsGain = 0.05f;   // 0.0 to 3.16 (linear gain)
	float m_lateGain = 0.05f;          // 0.0 to 10.0
	float m_airAbsorption = 0.994f;   // 0.892 to 1.0

	// Function pointers for EFX
	LPALGENEFFECTS alGenEffects = nullptr;
	LPALDELETEEFFECTS alDeleteEffects = nullptr;
	LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
	LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
	LPALEFFECTI alEffecti = nullptr;
	LPALEFFECTF alEffectf = nullptr;
	LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;

	bool LoadEFX();
	void UpdateEffect();

public:
	bool Init(ALCdevice* device);
	void AttachToSource(ALuint source);
	void DetachFromSource(ALuint source);
	void Cleanup();

	// Getters
	float GetDecayTime() const
	{
		return m_decayTime;
	}

	float GetReflectionsDelay() const
	{
		return m_reflectionsDelay;
	}

	float GetLateDelay() const
	{
		return m_lateDelay;
	}

	float GetRoomRolloff() const
	{
		return m_roomRolloff;
	}

	float GetDecayHFRatio() const
	{
		return m_decayHFRatio;
	}

	float GetReflectionsGain() const
	{
		return m_reflectionsGain;
	}

	float GetLateGain() const
	{
		return m_lateGain;
	}

	float GetAirAbsorption() const
	{
		return m_airAbsorption;
	}

	// Setters
	void SetDecayTime(float value);
	void SetReflectionsDelay(float value);
	void SetLateDelay(float value);
	void SetRoomRolloff(float value);
	void SetDecayHFRatio(float value);
	void SetReflectionsGain(float value);
	void SetLateGain(float value);
	void SetAirAbsorption(float value);

	// Preset functions
	void SetDefaultPreset();
	void SetSmallRoomPreset();
	void SetMediumRoomPreset();
	void SetLargeRoomPreset();
	void SetHallPreset();
	void SetCavePreset();

	~RoomReverb()
	{
		Cleanup();
	}
};
