#if !defined(BAREBONES_H)
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

// @params: timing, controller/keyboard input, bitmap buffer, sound buffer
internal_function void GameUpdateAndRender(
	game_offscreen_buffer* buffer,
	game_sound_output_buffer* soundBuffer,
	int toneHz);

#define BAREBONES_H
#endif