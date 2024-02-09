#if !defined(WIN32_BAREBONES_H)

struct win32_offscreen_buffer
{
	// pixels are always 32-bits wide, little endian 0x xx RR GG BB
	// memory order: BB GG RR xx
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int pitch;
};
struct win32_window_dimensions
{
	int width;
	int height;
};
struct win32_sound_output
{
	int samplesPerSec;
	uint32 runningSampleIndex;
	int bytesPerSample;
	int bufferSize;
	real32 tSine;
	int latencySample;
};

#define WIN32_BAREBONES_H
#endif