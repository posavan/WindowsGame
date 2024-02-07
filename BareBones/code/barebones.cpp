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

internal_function void GameUpdateAndRender(
	game_offscreen_buffer* buffer, 
	game_sound_output_buffer* soundBuffer,
	int toneHz)
{
	int blueOffset = 0;
	int greenOffset = 0;
	int redOffset = 0;

	GameOutputSound(soundBuffer, toneHz);
	RenderColor(buffer, blueOffset, greenOffset, redOffset);
}