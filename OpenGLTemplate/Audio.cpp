#include "Audio.h"
#include <math.h>
#include <cstdio>

#pragma comment(lib, "lib/fmod_vc.lib")

// Check for error
void FmodErrorCheck(FMOD_RESULT result)
{
	if (result != FMOD_OK) {
		const char* errorString = FMOD_ErrorString(result);
		// MessageBox(NULL, errorString, "FMOD Error", MB_OK);
		// Warning: error message commented out -- if headphones not plugged into computer in lab, error occurs
	}
}

float* ApplyZeroPadding(float* data, float* filter)
{
	//p = ceil((f-1) / 2)
	int filterSize = sizeof(*filter) / sizeof(float);
	int dataSize = sizeof(*data) / sizeof(float);
	int p = ceil((filterSize - 1) / 2);
	float* zeroPaddedData = new float[dataSize + p * 2];

	//calculate data size
	int zeroPaddedDataSize = sizeof(*zeroPaddedData) / sizeof(float);

	for (int i = 0; i < p; i++) {
		zeroPaddedData[i] = 0;
	}

	for (int i = p; i < zeroPaddedDataSize - p; i++) {
		zeroPaddedData[i] = data[i - p];
	}

	for (int i = zeroPaddedDataSize - p; i < zeroPaddedDataSize; i++) {
		zeroPaddedData[i] = 0;
	}

	data = zeroPaddedData;
	return zeroPaddedData;
}

/*
	Callback called when DSP is created.   This implementation creates a structure which is attached to the dsp state's 'plugindata' member.
*/FMOD_RESULT F_CALLBACK myDSPCreateCallback(FMOD_DSP_STATE* dsp_state)
{
	unsigned int blocksize = 512;
	FMOD_RESULT result;

	result = dsp_state->functions->getblocksize(dsp_state, &blocksize);
	FmodErrorCheck(result);

	mydsp_data_t* data = (mydsp_data_t*)calloc(sizeof(mydsp_data_t), 1);
	if (!data)
	{
		return FMOD_ERR_MEMORY;
	}
	dsp_state->plugindata = data;
	data->volume_linear = 1.0f;
	data->speed_percent = 1.0f;
	data->sample_count = blocksize;
	data->b_filter1 = { new float[21]{ -0.00349319,  0.00047716,  0.00459594,  0.00871522,  0.0126823,   0.01634645,
		0.01956573,  0.02221357,  0.02418469,  0.02540006,  0.02581071,  0.02540006,
		0.02418469,  0.02221357,  0.01956573,  0.01634645,  0.0126823,   0.00871522,
		0.00459594,  0.00047716, -0.00349319} };
	data->b_filter2 = { new float[21] {-0.01911611, -0.02526179, -0.02772793, -0.02595434, -0.02006462, -0.01086989,
		0.0002479,   0.01155558,  0.02125468,  0.02778399,  0.03008517,  0.02778399,
		0.02125468,  0.01155558,  0.0002479, -0.01086989, -0.02006462, -0.02595434,
		-0.02772793, -0.02526179, -0.01911611} };

	data->circ_buffer = (float*)malloc(blocksize * 8 * sizeof(float));      // *8 = maximum size allowing room for 7.1.   Could ask dsp_state->functions->getspeakermode for the right speakermode to get real speaker count.
	if (!data->circ_buffer)
	{
		return FMOD_ERR_MEMORY;
	}

	return FMOD_OK;
}


// DSP callback
FMOD_RESULT F_CALLBACK DSPCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels)
{
	mydsp_data_t* data = (mydsp_data_t*)dsp_state->plugindata;	//add data into our structure

	auto buffer_size = sizeof(*data->circ_buffer) / sizeof(float);
	//auto mean_length = buffer_size / inchannels;


	//ApplyZeroPadding(inbuffer, mixed_filt);

	if (buffer_size <= 0) return FMOD_ERR_MEMORY;

	for (unsigned int samp = 0; samp < length; samp++)	//run through sample length				
	{
		for (int chan = 0; chan < *outchannels; chan++)	//run through out channels length
		{
			// FIR Filter by change buffer size
			int circ_buf_pos = (data->sample_count * inchannels + chan) % buffer_size;
			outbuffer[samp * *outchannels + chan] = 1.0 * data->circ_buffer[circ_buf_pos]
				+ 1.0 * inbuffer[samp * inchannels + chan];
			data->circ_buffer[circ_buf_pos] = inbuffer[samp * inchannels + chan];
		}
		data->sample_count++;
	}

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK myDSPReleaseCallback(FMOD_DSP_STATE* dsp_state)
{
	if (dsp_state->plugindata)
	{
		mydsp_data_t* data = (mydsp_data_t*)dsp_state->plugindata;

		if (data->circ_buffer)
		{
			free(data->circ_buffer);
		}

		free(data);
	}

	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK myDSPGetParameterDataCallback(FMOD_DSP_STATE* dsp_state, int index, void** data, unsigned int* length, char*)
{
	if (index == 0)
	{
		unsigned int blocksize;
		FMOD_RESULT result;
		mydsp_data_t* mydata = (mydsp_data_t*)dsp_state->plugindata;

		result = dsp_state->functions->getblocksize(dsp_state, &blocksize);
		FmodErrorCheck(result);

		*data = (void*)mydata;
		*length = blocksize * 2 * sizeof(float);

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK myDSPSetParameterFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float value)
{
	if (index == 1)
	{
		mydsp_data_t* mydata = (mydsp_data_t*)dsp_state->plugindata;

		mydata->speed_percent = value;

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT F_CALLBACK myDSPGetParameterFloatCallback(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valstr)
{
	if (index == 1)
	{
		mydsp_data_t* mydata = (mydsp_data_t*)dsp_state->plugindata;

		*value = mydata->speed_percent;
		if (valstr)
		{
			sprintf(valstr, "%d", (int)((*value * 100.0f) + 0.5f));
		}

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
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
	
	// Create the DSP effect
	{
		FMOD_DSP_DESCRIPTION dspdesc;
		memset(&dspdesc, 0, sizeof(dspdesc));
		FMOD_DSP_PARAMETER_DESC wavedata_desc;
		FMOD_DSP_PARAMETER_DESC speed_desc;
		FMOD_DSP_PARAMETER_DESC* paramdesc[2] =
		{
			&wavedata_desc,
			&speed_desc
		};

		FMOD_DSP_INIT_PARAMDESC_DATA(wavedata_desc, "wave data", "", "wave data", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(speed_desc, "speed", "%", "speed in percent", 0, 1, 1);

		strncpy_s(dspdesc.name, "My first DSP unit", sizeof(dspdesc.name));
		dspdesc.numinputbuffers = 1;
		dspdesc.numoutputbuffers = 1;
		dspdesc.read = DSPCallback;
		dspdesc.create = myDSPCreateCallback;
		dspdesc.release = myDSPReleaseCallback;
		dspdesc.getparameterdata = myDSPGetParameterDataCallback;
		dspdesc.setparameterfloat = myDSPSetParameterFloatCallback;
		dspdesc.getparameterfloat = myDSPGetParameterFloatCallback;
		dspdesc.numparameters = 2;
		dspdesc.paramdesc = paramdesc;

		result = m_FmodSystem->createDSP(&dspdesc, &m_dsp);
		FmodErrorCheck(result);

		if (result != FMOD_OK) return false;
	}

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

	m_musicChannel->addDSP(0, m_dsp);


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

	m_musicChannel->addDSP(0, m_dsp);
	//m_musicChannel->addDSP(0, m_flange);
	//m_musicChannel->addDSP(0, m_lowpass);


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
	ToFMODVector(wall->getVertex(1), &wall1[1]);
	ToFMODVector(wall->getVertex(3), &wall1[2]);
	ToFMODVector(wall->getVertex(2), &wall1[3]);

	FMOD::Geometry* geometry;

	m_FmodSystem->createGeometry(1, 4, &geometry);

	int polyIndex = 0;

	geometry->addPolygon(1.f, 5.f, TRUE, 4, wall1, &polyIndex);

	FMOD_VECTOR wallPosition;
	glm::vec3 position = wall->getVertex(0);
	//glm::vec3 position = glm::vec3(wall1[2].x - wall1[0].x, wall1[1].y - wall1[0].y, wall1[0].z);

	ToFMODVector(position, &wallPosition);

	//geometry->setPosition(&wallPosition);
	geometry->setActive(TRUE);
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

	result = m_dsp->getBypass(&bypass);

	FmodErrorCheck(result);

}

void CAudio::FilterSwitch()
{
	if (bypass == true)
		result = m_dsp->setBypass(0);
	else if (bypass == false)
		result = m_dsp->setBypass(1);
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
