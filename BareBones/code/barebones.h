#if !defined(BAREBONES_H)

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
// Services that the platform layer provides to the game


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