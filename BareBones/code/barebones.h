// Services that the platform layer provides to the game


// Services that the game provides to the platform layer

struct offscreen_buffer
{
	// pixels are always 32-bits wide, little endian 0x xx RR GG BB
	// memory order: BB GG RR xx
	void* memory;
	int width;
	int height;
	int pitch;
};
// @params: timing, controller/keyboard input, bitmap buffer, sound buffer
internal_function void GameUpdateAndRender(offscreen_buffer);


#define BAREBONES_H
#endif