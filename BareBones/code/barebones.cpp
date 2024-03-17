#include "barebones.h"
#include <math.h>

void GameOutputSound(game_sound_output_buffer* soundBuffer, int toneHz)
{
	local_persist real32 tSine;
	int16 toneVolume = 600;
	int wavePeriod = soundBuffer->samplesPerSec / toneHz;
	int16* sampleOut = soundBuffer->samples;

	for (int SampleIndex = 0; SampleIndex < soundBuffer->sampleCount; ++SampleIndex)
	{
		real32 sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		//*sampleOut++ = sampleValue;//breaks here
		//*sampleOut++ = sampleValue;

		tSine += (2.0f * pi32 * 1.0f) / (real32)wavePeriod;
	}
}

void RenderColor(game_offscreen_buffer* buffer, int blueOffset, int greenOffset, int redOffset)
{

	uint8* row = (uint8*)buffer->memory;
	for (int y = 0; y < buffer->height; ++y)
	{
		uint32* pixel = (uint32*)row;
		for (int x = 0; x < buffer->width; ++x)
		{
			/*
			* Memory:	BB GG RR xx
			* Register:	xx RR GG BB
			*/
			uint8 blue = (x + blueOffset);
			uint8 green = (y + greenOffset);
			uint8 red = (0 + redOffset);

			*pixel++ = (blue | (green << 8) | (red << 16));
		}

		row += buffer->pitch;
	}
}

game_state* GameStartup(void)
{
	game_state* gameState = new game_state;
	if (gameState)
	{
		gameState->blueOffset = 0;
		gameState->greenOffset = 0;
		gameState->redOffset = 0;
		gameState->toneHz = 256;
	}

	return(gameState);
}

void DeleteGameState(game_state* gameState)
{
	//delete (gameState);
}

GAME_UPDATE_AND_RENDER(GameUpdateVideo)
{
	game_state* gameState = (game_state*)memory->permStorage;
	if (!memory->isInitialized)
	{
		gameState->toneHz = 256;

		memory->isInitialized = true;
	}

	for (int controlIndex = 0; controlIndex < ArrayCount(input->controllers); 
		++controlIndex)
	{
		game_controller_input* ctrlInput = &input->controllers[controlIndex];
		if (ctrlInput->isAnalog)
		{
			// use analog movement
			gameState->blueOffset += (int)(4.0f * (ctrlInput->stickAverageX));
			gameState->toneHz = 256 + (int)(128.0f * (ctrlInput->stickAverageY));
		}
		else
		{
			// use digital movement
			if (&ctrlInput->moveLeft.endedDown)
			{
				gameState->blueOffset -= 1;
			}
			if (&ctrlInput->moveRight.endedDown)
			{
				gameState->blueOffset += 1;
			}
		}

		if (ctrlInput->actDown.endedDown)
		{
			++gameState->greenOffset;
		}
	}

	RenderColor(buffer, gameState->blueOffset, gameState->greenOffset, gameState->redOffset);
}

GAME_GET_SOUND_SAMPLES(GameUpdateAudio)
{
	game_state* gameState = (game_state*)memory->permStorage;
	GameOutputSound(soundBuffer, gameState->toneHz);
}