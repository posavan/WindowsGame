
#include <stdint.h>

#pragma region Header
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

#include "barebones.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include <math.h>

#include "win32_barebones.h"

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
global_variable bool32 g_running;
global_variable win32_offscreen_buffer g_back_buffer;
global_variable LPDIRECTSOUNDBUFFER g_secondary_buffer;
#pragma endregion

internal_function void Win32LoadXInput()
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
			WAVEFORMATEX waveFormat = {};
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
						OutputDebugStringA("Primary buffer format was set.\n");
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
			if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &g_secondary_buffer, 0)))
			{
				OutputDebugStringA("Primary buffer format was set.\n");
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

internal_function void Win32ClearBuffer(win32_sound_output* soundOutput)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if (SUCCEEDED(g_secondary_buffer->Lock(
		0, soundOutput->bufferSize,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		uint8* destSample = (uint8*)Region1;
		for (DWORD byteIndex = 0; byteIndex < Region1Size; ++byteIndex)
		{
			*destSample++ = 0;
		}

		destSample = (uint8*)Region2;
		for (DWORD byteIndex = 0; byteIndex < Region2Size; ++byteIndex)
		{
			*destSample++ = 0;
		}

		g_secondary_buffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal_function void Win32FillSoundBuffer(
	win32_sound_output* soundOutput,
	game_sound_output_buffer* sourceBuffer,
	DWORD byteToLock,
	DWORD bytesToWrite)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(g_secondary_buffer->Lock(
		byteToLock, bytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		DWORD region1SampleCount = Region1Size / soundOutput->bytesPerSample;
		int16* destSample = (int16*)Region1;
		int16* srcSample = sourceBuffer->samples;
		for (DWORD SampleIndex = 0; SampleIndex < region1SampleCount; ++SampleIndex)
		{
			*destSample++ = *srcSample++;
			*destSample++ = *srcSample++;
			++soundOutput->runningSampleIndex;
		}

		DWORD region2SampleCount = Region2Size / soundOutput->bytesPerSample;
		destSample = (int16*)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < region2SampleCount; ++SampleIndex)
		{
			*destSample++ = *srcSample++;
			*destSample++ = *srcSample++;
			++soundOutput->runningSampleIndex;
		}

		g_secondary_buffer->Unlock(
			Region1, Region1Size,
			Region2, Region2Size);
	}
}

internal_function void Win32ProcessXInputDigitalButton(
	game_button_state* oldState,
	game_button_state* newState,
	DWORD XInputButtonState,
	DWORD buttonBit)
{
	newState->endedDown = ((XInputButtonState & buttonBit) == buttonBit);
	newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

internal_function win32_window_dimensions Win32GetWindowDimensions(HWND Window)
{
	win32_window_dimensions result{};

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	result.width = ClientRect.right - ClientRect.left;
	result.height = ClientRect.bottom - ClientRect.top;

	return(result);
}

internal_function void Win32ResizeDIBSection(win32_offscreen_buffer* buffer, int width, int height)
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
	buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = width * bytesPerPixel;

}

internal_function void Win32DisplayBufferToWindow(
	win32_offscreen_buffer* buffer,
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
		bool32 wasDown = ((LParam & (1 << 30)) != 0);
		bool32 isDown = ((LParam & (1 << 31)) == 0);
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
		win32_window_dimensions dimensions = Win32GetWindowDimensions(Window);

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
	LARGE_INTEGER perfCountFreqResult;
	QueryPerformanceFrequency(&perfCountFreqResult);
	int64 perfCountFrequency = perfCountFreqResult.QuadPart;

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

			// Sound Test
			win32_sound_output soundOutput = {};

			soundOutput.samplesPerSec = 48000;
			soundOutput.runningSampleIndex = 0;
			soundOutput.bytesPerSample = sizeof(int16) * 2;
			soundOutput.bufferSize = soundOutput.samplesPerSec * soundOutput.bytesPerSample;
			soundOutput.latencySample = soundOutput.samplesPerSec / 15;

			Win32InitDSound(Window, soundOutput.samplesPerSec, soundOutput.bufferSize);
			Win32ClearBuffer(&soundOutput);
			g_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
			int16* samples = (int16*)VirtualAlloc(0, soundOutput.bufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			game_memory gameMemory = {};
			gameMemory.permStorageSize = Megabytes(64);
			gameMemory.permStorage = VirtualAlloc(0, gameMemory.permStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			gameMemory.tempStorageSize = Gigabytes((uint64)4);
			gameMemory.tempStorage = VirtualAlloc(0, gameMemory.tempStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			if (samples && gameMemory.permStorage)
			{
				g_running = true;

				game_input gameInput[2] = {};
				game_input* newInput = &gameInput[0];
				game_input* oldInput = &gameInput[1];

				LARGE_INTEGER lastCounter;
				QueryPerformanceCounter(&lastCounter);
				uint64 lastCycleCount = __rdtsc();
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

					// Controller
					int maxControllerCount = XUSER_MAX_COUNT;
					if (maxControllerCount > ArrayCount(newInput->controllers))
					{
						maxControllerCount = ArrayCount(newInput->controllers);
					}
					for (DWORD ctrlIndex = 0; ctrlIndex < maxControllerCount; ++ctrlIndex)
					{
						game_controller_input* oldController = &oldInput->controllers[ctrlIndex];
						game_controller_input* newController = &newInput->controllers[ctrlIndex];

						XINPUT_STATE state;
						DWORD inputState = XInputGetState(ctrlIndex, &state);

						// Controller Mapping
						if (inputState == ERROR_SUCCESS)
						{
							// This controller is plugged in
							XINPUT_GAMEPAD* pad = &state.Gamepad;

							bool32 dPadUp = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
							bool32 dPadDown = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
							bool32 dPadLeft = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
							bool32 dPadRight = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

							newController->isAnalog = true;
							newController->startX = oldController->endX;
							newController->startY = oldController->endY;

							real32 X = 0;
							real32 max = 32767.0f;
							if (pad->sThumbLX < 0)
							{
								max += 1.0f;
							}
							X = (real32)pad->sThumbLX / max;
							newController->minX = newController->maxX = newController->endX = X;

							real32 Y = 0;
							max = 32767.0f;
							if (pad->sThumbLY < 0)
							{
								max += 1.0f;
							}
							Y = (real32)pad->sThumbLY / max;
							newController->minY = newController->maxY = newController->endY = Y;

							//int16 stickRX = (real32)pad->sThumbRX;
							//int16 stickRY = (real32)pad->sThumbRY;

							Win32ProcessXInputDigitalButton(
								&oldController->down, &newController->down,
								pad->wButtons, XINPUT_GAMEPAD_A);
							Win32ProcessXInputDigitalButton(
								&oldController->right, &newController->right,
								pad->wButtons, XINPUT_GAMEPAD_B);
							Win32ProcessXInputDigitalButton(
								&oldController->left, &newController->left,
								pad->wButtons, XINPUT_GAMEPAD_X);
							Win32ProcessXInputDigitalButton(
								&oldController->up, &newController->up,
								pad->wButtons, XINPUT_GAMEPAD_Y);
							Win32ProcessXInputDigitalButton(
								&oldController->leftShoulder, &newController->leftShoulder,
								pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
							Win32ProcessXInputDigitalButton(
								&oldController->rightShoulder, &newController->rightShoulder,
								pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);

							//Win32ProcessXInputDigitalButton(
							//	oldController->start, newController->start,
							//	pad->wButtons, XINPUT_GAMEPAD_START);
							//Win32ProcessXInputDigitalButton(
							//	oldController->back, newController->back,
							//	pad->wButtons, XINPUT_GAMEPAD_BACK);
							//Win32ProcessXInputDigitalButton(
							//	oldController->state, newController->state,
							//	pad->wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
							//Win32ProcessXInputDigitalButton(
							//	oldController->state, newController->state,
							//	pad->wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);

						}
						else
						{
							// This controller is not available
						}
					}

					DWORD byteToLock;
					DWORD targetCursor;
					DWORD bytesToWrite = 0;
					DWORD playCursor;
					DWORD writeCursor;
					bool32 soundIsValid = false;
					if (SUCCEEDED(g_secondary_buffer->GetCurrentPosition(&playCursor, &writeCursor)))
					{
						byteToLock = ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample)
							% soundOutput.bufferSize);
						targetCursor = playCursor +
							(soundOutput.latencySample * soundOutput.bytesPerSample) %
							soundOutput.bufferSize;

						if (byteToLock == playCursor)
						{
							bytesToWrite = 0;
						}
						else if (byteToLock > playCursor)
						{
							bytesToWrite = (soundOutput.bufferSize - byteToLock);
							bytesToWrite += playCursor;
						}
						else
						{
							bytesToWrite = targetCursor - byteToLock;
						}

						soundIsValid = true;
					}

					game_sound_output_buffer soundBuffer = {};
					soundBuffer.samplesPerSec = soundOutput.samplesPerSec;
					soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
					soundBuffer.samples = samples;

					game_offscreen_buffer buffer = {};
					buffer.memory = g_back_buffer.memory;
					buffer.width = g_back_buffer.width;
					buffer.height = g_back_buffer.height;
					buffer.pitch = g_back_buffer.pitch;
					GameUpdateAndRender(&gameMemory, &buffer, &soundBuffer, newInput);

					if (soundIsValid)
					{
						Win32FillSoundBuffer(&soundOutput, &soundBuffer, byteToLock, bytesToWrite);
					}

					win32_window_dimensions dimensions = Win32GetWindowDimensions(Window);
					Win32DisplayBufferToWindow(
						&g_back_buffer,
						DeviceContext,
						dimensions.width, dimensions.height);

					uint64 endCycleCount = __rdtsc();
					LARGE_INTEGER endCounter;
					QueryPerformanceCounter(&endCounter);

					// for debugging purposes
					uint64 cyclesElapsed = endCycleCount - lastCycleCount;
					int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
					real32 mSPerFrame = (((1000.0f * (real32)counterElapsed) / (real32)perfCountFrequency));
					real32 fps = (real32)perfCountFrequency / (real32)counterElapsed;
					real32 MegaCyclePerFrame = ((real32)cyclesElapsed / 1000000.0f);

					lastCounter = endCounter;
					lastCycleCount = endCycleCount;

					//Swap(oldInput, newInput);
					game_input* temp = newInput;
					newInput = oldInput;
					oldInput = temp;
				}
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

int WINAPI WinMainA(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CmdLine,
	int ShowCode)
{
	main(Instance);

	return(0);
}