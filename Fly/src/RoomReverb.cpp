#include "pch.h"

#include "RoomReverb.h"

void RoomReverb::SetDecayTime(float value)
{
	m_decayTime = std::clamp(value, 0.1f, 20.0f);
	if (m_effect)
	{
		alEffectf(m_effect, AL_REVERB_DECAY_TIME, m_decayTime);
		UpdateEffect();
	}
}

void RoomReverb::SetReflectionsDelay(float value)
{
	m_reflectionsDelay = std::clamp(value, 0.0f, 0.3f);
	if (m_effect)
	{
		alEffectf(m_effect, AL_REVERB_REFLECTIONS_DELAY, m_reflectionsDelay);
		UpdateEffect();
	}
}

void RoomReverb::SetLateDelay(float value)
{
	m_lateDelay = std::clamp(value, 0.0f, 0.1f);
	if (m_effect)
	{
		alEffectf(m_effect, AL_REVERB_LATE_REVERB_DELAY, m_lateDelay);
		UpdateEffect();
	}
}

void RoomReverb::SetRoomRolloff(float value)
{
	m_roomRolloff = std::clamp(value, 0.0f, 10.0f);
	if (m_effect)
	{
		alEffectf(m_effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, m_roomRolloff);
		UpdateEffect();
	}
}

void RoomReverb::SetDecayHFRatio(float value)
{
	m_decayHFRatio = std::clamp(value, 0.1f, 2.0f);
	if (m_effect)
	{
		alEffectf(m_effect, AL_REVERB_DECAY_HFRATIO, m_decayHFRatio);
		UpdateEffect();
	}
}

void RoomReverb::SetReflectionsGain(float value)
{
	m_reflectionsGain = std::clamp(value, 0.0f, 3.16f);
	if (m_effect)
	{
		alEffectf(m_effect, AL_REVERB_REFLECTIONS_GAIN, m_reflectionsGain);
		UpdateEffect();
	}
}

void RoomReverb::SetLateGain(float value)
{
	m_lateGain = std::clamp(value, 0.0f, 10.0f);
	if (m_effect)
	{
		alEffectf(m_effect, AL_REVERB_LATE_REVERB_GAIN, m_lateGain);
		UpdateEffect();
	}
}

void RoomReverb::SetAirAbsorption(float value)
{
	m_airAbsorption = std::clamp(value, 0.892f, 1.0f);
	if (m_effect)
	{
		alEffectf(m_effect, AL_REVERB_AIR_ABSORPTION_GAINHF, m_airAbsorption);
		UpdateEffect();
	}
}

void RoomReverb::SetDefaultPreset()
{
	// Simulate it as if we werent even applying any effects
	SetDecayTime(1.0f);
	SetReflectionsDelay(0.02f);
	SetLateDelay(0.03f);
	SetRoomRolloff(0.0f);
	SetDecayHFRatio(1.0f);
	SetReflectionsGain(0.05f);
	SetLateGain(0.05f);
	SetAirAbsorption(0.994f);
}

void RoomReverb::SetSmallRoomPreset()
{
	// Small room characteristics:
	// - Short decay time due to close walls
	// - Early reflections arrive quickly
	// - Stronger early reflections due to close walls
	// - Less air absorption due to small space
	SetDecayTime(0.5f);         // Short reverb tail
	SetReflectionsDelay(0.02f); // Quick early reflections
	SetLateDelay(0.03f);        // Short gap between early and late
	SetRoomRolloff(0.6f);       // Sound dies quickly with distance
	SetDecayHFRatio(0.85f);     // Slightly less high frequency decay
	SetReflectionsGain(1.2f);   // Strong early reflections
	SetLateGain(0.8f);          // Moderate late reverb
	SetAirAbsorption(0.994f);   // Standard air absorption
}

void RoomReverb::SetMediumRoomPreset()
{
	// Medium room characteristics:
	// - Balanced decay time
	// - Moderate reflection timings
	// - Natural balance of early and late reflections
	SetDecayTime(1.8f);         // Moderate reverb tail
	SetReflectionsDelay(0.03f); // Slightly delayed early reflections
	SetLateDelay(0.04f);        // Natural gap
	SetRoomRolloff(0.4f);       // Moderate distance attenuation
	SetDecayHFRatio(0.9f);      // Natural high frequency decay
	SetReflectionsGain(0.9f);   // Moderate early reflections
	SetLateGain(1.0f);          // Balanced late reverb
	SetAirAbsorption(0.994f);   // Standard air absorption
}

void RoomReverb::SetLargeRoomPreset()
{
	// Large room characteristics:
	// - Longer decay time
	// - Delayed reflections due to distance
	// - More distinct early and late reflections
	SetDecayTime(2.8f);         // Long reverb tail
	SetReflectionsDelay(0.05f); // Delayed early reflections
	SetLateDelay(0.06f);        // Longer gap for large space
	SetRoomRolloff(0.3f);       // Gradual distance attenuation
	SetDecayHFRatio(0.95f);     // More natural high frequency decay
	SetReflectionsGain(0.7f);   // Softer early reflections
	SetLateGain(1.2f);          // Stronger late reverb
	SetAirAbsorption(0.992f);   // Slightly more air absorption
}

void RoomReverb::SetHallPreset()
{
	// Concert hall characteristics:
	// - Long decay time for rich sound
	// - Distinct early reflections
	// - Strong late reverb for fullness
	SetDecayTime(3.5f);         // Extended reverb tail
	SetReflectionsDelay(0.06f); // Delayed early reflections
	SetLateDelay(0.08f);        // Clear separation
	SetRoomRolloff(0.2f);       // Smooth distance attenuation
	SetDecayHFRatio(1.0f);      // Natural high frequency decay
	SetReflectionsGain(0.6f);   // Subtle early reflections
	SetLateGain(1.5f);          // Rich late reverb
	SetAirAbsorption(0.990f);   // Increased air absorption
}

void RoomReverb::SetCavePreset()
{
	// Cave characteristics:
	// - Very long decay time
	// - Sparse early reflections
	// - Strong late reverb
	// - High air absorption
	SetDecayTime(5.0f);         // Very long reverb tail
	SetReflectionsDelay(0.15f); // Sparse, delayed early reflections
	SetLateDelay(0.09f);        // Long reflection pattern
	SetRoomRolloff(0.1f);       // Very gradual distance attenuation
	SetDecayHFRatio(1.2f);      // Enhanced high frequency decay
	SetReflectionsGain(0.4f);   // Weak early reflections
	SetLateGain(2.0f);          // Very strong late reverb
	SetAirAbsorption(0.985f);   // High air absorption
}

bool RoomReverb::LoadEFX()
{
	if (alcIsExtensionPresent(m_device, "ALC_EXT_EFX") == AL_FALSE)
	{
		return false;
	}

	alGenEffects = (LPALGENEFFECTS) alGetProcAddress("alGenEffects");
	alDeleteEffects = (LPALDELETEEFFECTS) alGetProcAddress("alDeleteEffects");
	alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS) alGetProcAddress("alGenAuxiliaryEffectSlots");
	alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS) alGetProcAddress("alDeleteAuxiliaryEffectSlots");
	alEffecti = (LPALEFFECTI) alGetProcAddress("alEffecti");
	alEffectf = (LPALEFFECTF) alGetProcAddress("alEffectf");
	alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI) alGetProcAddress("alAuxiliaryEffectSloti");

	return (alGenEffects && alDeleteEffects && alGenAuxiliaryEffectSlots && alDeleteAuxiliaryEffectSlots && alEffecti && alEffectf && alAuxiliaryEffectSloti);
}

void RoomReverb::UpdateEffect()
{
	if (m_slot && m_effect)
	{
		alAuxiliaryEffectSloti(m_slot, AL_EFFECTSLOT_EFFECT, m_effect);
	}
}

bool RoomReverb::Init(ALCdevice* device)
{
	m_device = device;

	if (!LoadEFX())
	{
		return false;
	}

	// Generate effect
	alGenEffects(1, &m_effect);
	if (alGetError() != AL_NO_ERROR)
	{
		return false;
	}

	// Set effect type to reverb
	alEffecti(m_effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
	if (alGetError() != AL_NO_ERROR)
	{
		alDeleteEffects(1, &m_effect);
		return false;
	}

	// Generate auxiliary effect slot
	alGenAuxiliaryEffectSlots(1, &m_slot);
	if (alGetError() != AL_NO_ERROR)
	{
		alDeleteEffects(1, &m_effect);
		return false;
	}

	// Configure for empty room reverb
	// Decay time - medium-sized room
	alEffectf(m_effect, AL_REVERB_DECAY_TIME, m_decayTime);
	// Early reflections - closer together for small room
	alEffectf(m_effect, AL_REVERB_REFLECTIONS_DELAY, m_reflectionsDelay);
	// Late reverb - slightly delayed
	alEffectf(m_effect, AL_REVERB_LATE_REVERB_DELAY, m_lateDelay);
	// Room size factor
	alEffectf(m_effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, m_roomRolloff);
	// High-frequency decay ratio
	alEffectf(m_effect, AL_REVERB_DECAY_HFRATIO, m_decayHFRatio);
	// Reflection level
	alEffectf(m_effect, AL_REVERB_REFLECTIONS_GAIN, m_reflectionsGain);
	// Late reverb level
	alEffectf(m_effect, AL_REVERB_LATE_REVERB_GAIN, m_lateGain);
	// Air absorption
	alEffectf(m_effect, AL_REVERB_AIR_ABSORPTION_GAINHF, m_airAbsorption);

	// Attach effect to slot
	alAuxiliaryEffectSloti(m_slot, AL_EFFECTSLOT_EFFECT, m_effect);
	if (alGetError() != AL_NO_ERROR)
	{
		Cleanup();
		return false;
	}

	return true;
}

void RoomReverb::AttachToSource(ALuint source)
{
	if (m_slot)
	{
		// Connect the source to the effect slot
		alSource3i(source, AL_AUXILIARY_SEND_FILTER, m_slot, 0, AL_FILTER_NULL);
	}
}

void RoomReverb::DetachFromSource(ALuint source)
{
	// Disconnect the source from the effect slot
	alSource3i(source, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
}

void RoomReverb::Cleanup()
{
	if (m_slot)
	{
		alDeleteAuxiliaryEffectSlots(1, &m_slot);
		m_slot = 0;
	}
	if (m_effect)
	{
		alDeleteEffects(1, &m_effect);
		m_effect = 0;
	}
}
