#include "barebones.h"

internal_function void RenderColor(offscreen_buffer* buffer, int blueOffset, int greenOffset, int redOffset)
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

internal_function void GameUpdateAndRender(offscreen_buffer* buffer)
{
	int blueOffset = 0;
	int greenOffset = 0;
	int redOffset = 0;
	RenderColor(buffer, blueOffset, greenOffset, redOffset);
}