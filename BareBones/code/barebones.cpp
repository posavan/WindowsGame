#include "barebones.h"
#include <math.h>

internal_function void GameOutputSound(game_sound_output_buffer* soundBuffer, int toneHz)
{
	local_persist real32 tSine;
	int16 toneVolume = 600;
	int wavePeriod = soundBuffer->samplesPerSec / toneHz;
	int16* sampleOut = soundBuffer->samples;

	for (int SampleIndex = 0; SampleIndex < soundBuffer->sampleCount; ++SampleIndex)
	{
		real32 sineValue = sinf(tSine);
		int16 sampleValue = (int16)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		tSine += (2.0f * pi32 * 1.0f) / (real32)wavePeriod;
	}
}

internal_function void RenderColor(game_offscreen_buffer* buffer, int blueOffset, int greenOffset, int redOffset)
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

internal_function game_state* GameStartup(void)
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

internal_function void DeleteGameState(game_state* gameState)
{
	//delete (gameState);
}

internal_function void GameUpdateAndRender(
	game_memory* memory,
	game_offscreen_buffer* buffer,
	game_sound_output_buffer* soundBuffer,
	game_input* gameInput)
{
	game_state* gameState = (game_state*)memory->permStorage;
	if (!memory->isInitialized)
	{
		gameState->toneHz = 256;

		memory->isInitialized = true;
	}
	game_controller_input* input0 = &gameInput->controllers[0];
	if (input0->isAnalog)
	{
		// use analog movement
		gameState->blueOffset += (int)(4.0f * (input0->endX));
		gameState->toneHz = 256 + (int)(128.0f * (input0->endY));
	}
	else
	{
		// use digital movement
	}

	if (input0->down.endedDown)
	{
		++gameState->greenOffset;
	}

	GameOutputSound(soundBuffer, gameState->toneHz);
	RenderColor(buffer, gameState->blueOffset, gameState->greenOffset, gameState->redOffset);
}