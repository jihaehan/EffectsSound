#include "Audio.h"
#include <math.h>
#include <cstdio>

#pragma comment(lib, "lib/fmod_vc.lib")

// Check for error
void FmodErrorCheck(FMOD_RESULT result)
{
	if (result != FMOD_OK) {
		const char *errorString = FMOD_ErrorString(result);
		// MessageBox(NULL, errorString, "FMOD Error", MB_OK);
		// Warning: error message commented out -- if headphones not plugged into computer in lab, error occurs
	}
}


CAudio::CAudio()
{
	listenerVelocity.x = 1;
	listenerVelocity.y = 1;
	listenerVelocity.z = 1;
	listenerUp.x = 0.f;
	listenerUp.y = 1.f;
	listenerUp.z = 0;
	listenerForward.x = 0.f;
	listenerForward.y = 0.f;
	listenerForward.z = 1.0;
	listenerPos.x = 3.f;
	listenerPos.y = 3.f;
	listenerPos.z = 1.f;
	soundPosition.x = 3.f;
	soundPosition.y = 3.f;
	soundPosition.z = 1.0;
	soundVelocity.x = 1;
	soundVelocity.y = 1;
	soundVelocity.z = 1.0;
}

CAudio::~CAudio()
{}

bool CAudio::Initialise()
{
	// Create an FMOD system
	result = FMOD::System_Create(&m_FmodSystem);
	FmodErrorCheck(result);
	if (result != FMOD_OK) 
		return false;

	// Initialise the system
	result = m_FmodSystem->init(32, FMOD_INIT_NORMAL, 0);
	FmodErrorCheck(result);
	if (result != FMOD_OK) 
		return false;
	
	return true;
}

// Load an event sound
bool CAudio::LoadEventSound(char *filename)
{
	result = m_FmodSystem->createSound(filename, FMOD_LOOP_OFF, 0, &m_eventSound);
	FmodErrorCheck(result);
	if (result != FMOD_OK) 
		return false;

	return true;

}

// Play an event sound
bool CAudio::PlayEventSound()
{
	result = m_FmodSystem->playSound(m_eventSound, NULL, false, NULL);
	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;
	return true;
}


// Load a music stream
bool CAudio::LoadMusicStream(char *filename)
{
	result = m_FmodSystem->createStream(filename, NULL | FMOD_LOOP_NORMAL, 0, &m_music);
	FmodErrorCheck(result);

	if (result != FMOD_OK) 
		return false;

	return true;
}


// Play a music stream
bool CAudio::PlayMusicStream()
{
	result = m_FmodSystem->playSound(m_music, NULL, false, &m_musicChannel);
	FmodErrorCheck(result);

	if (result != FMOD_OK)
		return false;


	return true;
}



bool CAudio::Load3DSound(char* filename)
{

	result = m_FmodSystem->createSound(filename, FMOD_3D, 0, &m_eventSound);
	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;

	//set 3D settings for spatialized sound
	result = m_FmodSystem->set3DSettings(1.0, 0.5, 1.0);
	m_eventSound->set3DMinMaxDistance(1.f, 5000.f);

	return true;

}

void CAudio::Play3DSound()
{
	// Play the sound
	result = m_FmodSystem->playSound(m_eventSound, NULL, false, &m_musicChannel);
	FMOD_VECTOR pos = { 0.0f, 0.0f, 0.0f };
	FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

	result = m_musicChannel->set3DAttributes(&pos, &vel);	 // The the 3D position of the sound
	result = m_musicChannel->setVolume(1.0);
	m_musicChannel->addDSP(0, m_flange);
	m_musicChannel->addDSP(0, m_lowpass);


	FmodErrorCheck(result);
}

void CAudio::UpdateListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 forward, glm::vec3 up)
{

	listenerVelocity.x = velocity.x;
	listenerVelocity.y = velocity.y;
	listenerVelocity.z = velocity.z;

	listenerPos.x = position.x;
	listenerPos.y = position.y;
	listenerPos.z = position.z;

	listenerForward.x = forward.x;
	listenerForward.y = forward.y;
	listenerForward.z = forward.z;

	listenerUp.x = up.x;
	listenerUp.y = up.y;
	listenerUp.z = up.z;

	result = m_FmodSystem->set3DListenerAttributes(0, &listenerPos, &listenerVelocity, &listenerForward, &listenerUp);
	FmodErrorCheck(result);

}

void CAudio::Update3DSound(glm::vec3 position, glm::vec3 velocity)
{
	soundPosition.x = position.x;
	soundPosition.y = position.y;
	soundPosition.z = position.z;

	soundVelocity.x = velocity.x;
	soundVelocity.y = velocity.y;
	soundVelocity.z = velocity.z;

	result = m_musicChannel->set3DAttributes(&soundPosition, &soundVelocity);
}


bool CAudio::CreateLowPass()
{
	result = m_FmodSystem->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &m_lowpass);
	result = m_musicChannel->addDSP(0, m_lowpass);

	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;

	return true;
}

bool CAudio::SetLowPass(float freq)
{
	result = m_lowpass->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, freq);
	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;

	return true;
}

bool CAudio::CreateFlange()
{
	result = m_FmodSystem->createDSPByType(FMOD_DSP_TYPE_FLANGE, &m_flange);
	result = m_musicChannel->addDSP(0, m_flange);

	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;

	return true;
}

bool CAudio::SetFlangeDepth(float depth)
{
	result = m_flange->setParameterFloat(FMOD_DSP_FLANGE_DEPTH, depth);
	FmodErrorCheck(result);
	if (result != FMOD_OK)
		return false;

	return true;
}

void CAudio::CreateObstacle(Wall* wall)
{
	FMOD_VECTOR wall1[4];

	ToFMODVector(wall->getVertex(0), &wall1[0]);
	ToFMODVector(wall->getVertex(1), &wall1[0]);
	ToFMODVector(wall->getVertex(2), &wall1[0]);
	ToFMODVector(wall->getVertex(3), &wall1[0]);

	FMOD::Geometry* geometry;

	result = m_FmodSystem->createGeometry(1, 4, &geometry);

	int polyIndex = 0;

	result = geometry->addPolygon(0.5f, 0.5f, TRUE, 4, wall1, &polyIndex);

	FMOD_VECTOR wallPosition;
	glm::vec3 position = glm::vec3(wall1[2].x - wall1[0].x, wall1[1].y - wall1[0].y, wall1[0].z);

	ToFMODVector(position, &wallPosition);

	result = geometry->setPosition(&wallPosition);
	result = geometry->setActive(TRUE);
}


void CAudio::ToFMODVector(glm::vec3 vec, FMOD_VECTOR* fVec)
{
	fVec->x = vec.x;
	fVec->y = vec.y;
	fVec->z = vec.z;
}

void CAudio::Update(float dt)
{
	m_FmodSystem->update();

	FmodErrorCheck(result);

}

void CAudio::FilterSwitch()
{
	if (bypass == true)
		result = m_flange->setBypass(0);
	else if (bypass == false)
		result = m_flange->setBypass(1);
	FmodErrorCheck(result);
	
}

void CAudio::SpeedDown(float &speedpercent)
{
	result = m_dsp->getParameterFloat(1, &speedpercent, 0, 0);
	FmodErrorCheck(result);

	if (speedpercent > 0.0f)
	{
		speedpercent -= 0.05f;
	}

	result = m_dsp->setParameterFloat(1, speedpercent);
	FmodErrorCheck(result);

}

void CAudio::SpeedUp(float &speedpercent)
{
	result = m_dsp->getParameterFloat(1, &speedpercent, 0, 0);
	FmodErrorCheck(result);

	if (speedpercent < 1.0f)
	{
		speedpercent += 0.05f;
	}

	result = m_dsp->setParameterFloat(1, speedpercent);
	FmodErrorCheck(result);

}
