#if !defined(BAREBONES_H)

/*
* BAREBONES_INTERNAL:
*	0 - Build for public release
*	1 - Build for development
*
* BAREBONES_SLOW:
*	0 - No slow code allowed
*	1 - Slow code welcome
*/
#if BAREBONES_SLOW
#define Assert(Expression) if(!(Expression)) {*(int*)0 = 0;}//if false, breaks program
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// Services that the platform layer provides to the game
#if BAREBONES_INTERNAL
struct debug_read_file_result
{
	uint32 ContentsSize;
	void* Contents;
};

internal_function debug_read_file_result DEBUGPlatformReadEntireFile(char* filename);
internal_function void DEBUGPlatformFreeFileMemory(void* BitmapMemory);
internal_function bool32 DEBUGPlatformWriteEntireFile(char* filename, 
	uint32 memorySize, void* memory);
#endif

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
	int16* samples;
};
struct game_button_state
{
	int halfTransitionCount;
	bool32 endedDown;
};
struct game_controller_input
{
	real32 isAnalog;

	real32 startX;
	real32 startY;

	real32 minX;
	real32 minY;

	real32 maxX;
	real32 maxY;

	real32 endX;
	real32 endY;

	union
	{
		game_button_state buttons[6];
		struct
		{
			game_button_state up;
			game_button_state down;
			game_button_state left;
			game_button_state right;
			game_button_state leftShoulder;
			game_button_state rightShoulder;
		};
	};
};
struct game_input
{
	real32 game_clock;
	game_controller_input controllers[4];
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

#define BAREBONES_H
#endif