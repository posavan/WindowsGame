#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#pragma region Header
#define internal_function static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct offscreen_buffer
{
	// pixels are always 32-bits wide, little endian 0x xx RR GG BB
	// memory order: BB GG RR xx
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int pitch;
};
struct window_dimensions
{
	int width;
	int height;
};
#pragma endregion

#pragma region Controller
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(x_input_get_state);

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(x_input_set_state);

XINPUT_GET_STATE(XInputGetStateStub) { return(ERROR_DEVICE_NOT_CONNECTED); }
XINPUT_SET_STATE(XInputSetStateStub) { return(ERROR_DEVICE_NOT_CONNECTED); }

global_variable x_input_get_state* xInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state* xInputSetState_ = XInputSetStateStub;

#define XInputGetState xInputGetState_
#define XInputSetState xInputSetState_
#pragma endregion

#pragma region Sound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#pragma endregion

#pragma region Globals
global_variable bool g_running;
global_variable offscreen_buffer g_back_buffer;
#pragma endregion

internal_function void Win32LoadXInput(void)
{
	HMODULE library = LoadLibraryA("xinput1_4.dll");
	if (library)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(library, "XInputGetState");
		if (!XInputGetState) {
			XInputGetState = XInputGetStateStub;
		}
		xInputSetState_ = (x_input_set_state*)GetProcAddress(library, "XInputSetState");
		if (!XInputSetState) {
			XInputSetState = XInputSetStateStub;
		}
		else
		{
			// log diagnostic
		}
	}
	else
	{
		// log diagnostic
	}
}

internal_function void Win32InitDSound(HWND Window, int32 samplesPerSecond, int32 bufferSize)
{
	HMODULE library = LoadLibraryA("dsound.dll");
	if (library)
	{
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(library, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat;
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = samplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if (SUCCEEDED(directSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC bufferDescription = {};
				bufferDescription.dwSize = sizeof(bufferDescription);
				bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
				{
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
					{

					}
					else
					{
						// log diagnostic
					}
				}
				else
				{
					// log diagnostic
				}
			}
			else
			{
				// log diagnostic
			}

			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			LPDIRECTSOUNDBUFFER secondaryBuffer;
			if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, 0)))
			{

			}
			else
			{
				// log diagnostic
			}
		}
		else
		{
			// log diagnostic
		}
	}
	else
	{
		// log diagnostic
	}

}

internal_function window_dimensions Win32GetWindowDimensions(HWND Window)
{
	window_dimensions result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	result.width = ClientRect.right - ClientRect.left;
	result.height = ClientRect.bottom - ClientRect.top;

	return(result);
}

internal_function void Win32RenderColor(offscreen_buffer* buffer, int blueOffset, int greenOffset)
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
			uint8 red = (0);

			*pixel++ = (blue | (green << 8));// | (red << 16));
		}

		row += buffer->pitch;
	}
}

internal_function void Win32ResizeDIBSection(offscreen_buffer* buffer, int width, int height)
{
	if (buffer->memory)
	{
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;
	int bytesPerPixel = 4;

	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = (buffer->width * buffer->height) * bytesPerPixel;
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = width * bytesPerPixel;

}

internal_function void Win32DisplayBufferToWindow(
	offscreen_buffer* buffer,
	HDC DeviceContext,
	int windowWidth, int windowHeight
)
{
	StretchDIBits(
		DeviceContext,
		/* dest: x, y, width, height,
		   src:	 x, y, width, height,*/
		0, 0, windowWidth, windowHeight,
		0, 0, buffer->width, buffer->height,
		buffer->memory,
		&buffer->info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

internal_function LRESULT CALLBACK Win32MainWindowCallback(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam)
{
	LRESULT result = 0;

	switch (Message)
	{
	case WM_CLOSE:
	{
		g_running = false;
		OutputDebugStringA("WM_CLOSE\n");
	} break;
	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	} break;
	case WM_DESTROY:
	{
		g_running = false;
		OutputDebugStringA("WM_DESTROY\n");
	} break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32 VKCode = WParam;
		bool wasDown = ((LParam & (1 << 30)) != 0);
		bool isDown = ((LParam & (1 << 31)) == 0);
		if (wasDown != isDown)
		{
			if (VKCode == 'W')
			{
				OutputDebugStringA("W: ");
				if (isDown)
				{
					OutputDebugStringA("isDown ");
				}
				else if (wasDown)
				{
					OutputDebugStringA("wasDown ");
				}
				OutputDebugStringA("\n");
			}
			else if (VKCode == 'A')
			{
			}
			else if (VKCode == 'S')
			{
			}
			else if (VKCode == 'D')
			{
			}
			else if (VKCode == 'Q')
			{
			}
			else if (VKCode == 'E')
			{
			}
			else if (VKCode == VK_UP)
			{
			}
			else if (VKCode == VK_LEFT)
			{
			}
			else if (VKCode == VK_DOWN)
			{
			}
			else if (VKCode == VK_RIGHT)
			{
			}
			else if (VKCode == VK_ESCAPE)
			{
			}
			else if (VKCode == VK_SPACE)
			{
			}
		}

		bool32 AltKeyDown = (LParam & (1 << 29));
		if ((VKCode == VK_F4) && AltKeyDown) 
		{ 
			g_running = false; 
		}

	} break;

	case WM_PAINT:
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);
		window_dimensions dimensions = Win32GetWindowDimensions(Window);

		Win32DisplayBufferToWindow(
			&g_back_buffer,
			DeviceContext,
			dimensions.width, dimensions.height);
		EndPaint(Window, &Paint);
	} break;

	default:
	{
		result = DefWindowProcA(Window, Message, WParam, LParam);
	} break;
	}

	return(result);
}

int main(HINSTANCE Instance) {
	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	Win32ResizeDIBSection(&g_back_buffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "BareBonesWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Bare Bones",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0);
		if (Window)
		{
			HDC DeviceContext = GetDC(Window);

			int xOffset = 0;
			int yOffset = 0;

			Win32InitDSound(Window, 48000, 48000*sizeof(int16)*2);

			g_running = true;
			while (g_running)
			{
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						g_running = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				for (DWORD ctrlIndex = 0; ctrlIndex < XUSER_MAX_COUNT; ++ctrlIndex)
				{
					XINPUT_STATE state;
					DWORD inputState = XInputGetState(ctrlIndex, &state);
					if (inputState == ERROR_SUCCESS)
					{
						// This controller is plugged in
						XINPUT_GAMEPAD* pad = &state.Gamepad;

						bool dPadUp = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool dPadDown = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool dPadLeft = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool dPadRight = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool buttonStart = (pad->wButtons & XINPUT_GAMEPAD_START);
						bool buttonBack = (pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool thumbLeft = (pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
						bool thumbRight = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
						bool shoulderLeft = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool shoulderRight = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool buttonA = (pad->wButtons & XINPUT_GAMEPAD_A);
						bool buttonB = (pad->wButtons & XINPUT_GAMEPAD_B);
						bool buttonX = (pad->wButtons & XINPUT_GAMEPAD_X);
						bool buttonY = (pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 stickLX = pad->sThumbLX;
						int16 stickLY = pad->sThumbLY;
						int16 stickRX = pad->sThumbRX;
						int16 stickRY = pad->sThumbRY;

						xOffset += stickLX >> 12;
						yOffset += stickLY >> 12;
					}
					else
					{
						// This controller is not available
					}
				}

				//add rumble
				XINPUT_VIBRATION vibrate;
				//vibrate.wLeftMotorSpeed = 50000;
				//vibrate.wRightMotorSpeed = 50000;
				XInputSetState(0, &vibrate);

				Win32RenderColor(&g_back_buffer, xOffset, yOffset);
				window_dimensions dimensions = Win32GetWindowDimensions(Window);
				Win32DisplayBufferToWindow(
					&g_back_buffer,
					DeviceContext,
					dimensions.width, dimensions.height);

			}
		}
		else
		{
			//log
		}
	}
	else
	{
		//log
	}

}

int WINAPI WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CmdLine,
	int ShowCode)
{
	main(Instance);

	return(0);
}