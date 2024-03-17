#ifndef BAREBONES_H
#define BAREBONES_H

//#include "handmade_platform.h"
//#include "handmade_intrinsic.h"
//#include "handmade_math.h"

#define PI32 3.14159265359

#include <math.h>
#include <stdint.h>

#define internal_function static
#define local_persist static
#define global_variable static

#define pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

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

// @params: timing, controller/keyboard input, bitmap buffer
#define GAME_UPDATE_AND_RENDER(name) void name( \
	game_memory* memory, game_input* input, \
	game_offscreen_buffer* buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_video);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}

#define GAME_GET_SOUND_SAMPLES(name) void name( \
	game_memory* memory, \
	game_sound_output_buffer* soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_update_audio);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
{
}

struct game_state
{
	int toneHz;
	int blueOffset;
	int greenOffset;
	int redOffset;
};

#endif