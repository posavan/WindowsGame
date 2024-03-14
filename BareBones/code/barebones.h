#ifndef BAREBONES_H
#define BAREBONES_H

//#include "handmade_platform.h"
//#include "handmade_intrinsic.h"
//#include "handmade_math.h"

#define PI32 3.14159265359

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// Services that the game provides to the platform layer

struct game_offscreen_buffer
{
	// pixels are always 32-bits wide, little endian 0x xx RR GG BB
	// memory order: BB GG RR xx
	void* memory;
	int width;
	int height;
	int pitch;
};
struct game_sound_output_buffer
{
	int samplesPerSec;
	int sampleCount;
	int toneVolume;
	int16 *samples;
	int16* memory;
};
struct game_button_state
{
	int halfTransitionCount;
	bool32 endedDown;
};
struct game_controller_input
{
	bool32 isConnected;
	real32 isAnalog;
	real32 stickAverageX;
	real32 stickAverageY;

	union
	{
		game_button_state buttons[12];
		struct
		{
			game_button_state moveUp;
			game_button_state moveDown;
			game_button_state moveLeft;
			game_button_state moveRight;

			game_button_state actUp;
			game_button_state actDown;
			game_button_state actLeft;
			game_button_state actRight;

			game_button_state leftShoulder;
			game_button_state rightShoulder;

			game_button_state start;
			game_button_state back;
		};
	};
};
struct game_input
{
	real32 game_clock;
	game_controller_input controllers[5];
};
struct game_memory
{
	bool32 isInitialized;
	uint64 permStorageSize;
	void* permStorage;
	uint64 tempStorageSize;
	void* tempStorage;
};

// @params: timing, controller/keyboard input, bitmap buffer, sound buffer
internal_function void GameUpdateAndRender(
	game_memory* memory,
	game_offscreen_buffer* buffer,
	game_sound_output_buffer* soundBuffer,
	game_input* gameInput);

struct game_state
{
	int toneHz;
	int blueOffset;
	int greenOffset;
	int redOffset;
};

#endif