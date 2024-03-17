#pragma region Header

#include "barebones.h"

#include <windows.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_barebones.h"
#pragma endregion

#pragma region Globals
global_variable bool g_running = true;
global_variable bool g_debug_pause_render = false;
global_variable win32_offscreen_buffer g_back_buffer;
global_variable LPDIRECTSOUNDBUFFER g_sound_buffer;
global_variable uint64 g_perf_count_freq;
global_variable bool g_show_cursor;
#pragma endregion

#pragma region Controller
#define XINPUT_GET_STATE(name) \
DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(x_input_get_state);

#define XINPUT_SET_STATE(name) \
DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(x_input_set_state);

XINPUT_GET_STATE(XInputGetStateStub) { return(ERROR_DEVICE_NOT_CONNECTED); }
XINPUT_SET_STATE(XInputSetStateStub) { return(ERROR_DEVICE_NOT_CONNECTED); }

global_variable x_input_get_state* xInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state* xInputSetState_ = XInputSetStateStub;

#define XInputGetState xInputGetState_
#define XInputSetState xInputSetState_
#pragma endregion

#pragma region Sound
#define DIRECT_SOUND_CREATE(name) \
HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#pragma endregion

inline LARGE_INTEGER Win32GetWallClock()
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
	real32 result = (real32)(end.QuadPart - start.QuadPart) /
		(real32)g_perf_count_freq;
	return result;
}

internal_function win32_game_code Win32LoadGameCode()
{
	win32_game_code result = {};

	CopyFileA("barebones.exe", "barebones_temp.dll", FALSE);
	result.gameDLL = LoadLibraryA("barebones_temp.dll");
	if (result.gameDLL)
	{
		result.gameUpdateVideo = (game_update_video*)GetProcAddress(result.gameDLL, "GameUpdateAndRender");
		result.gameUpdateAudio = (game_update_audio*)GetProcAddress(result.gameDLL, "GameGetSoundSamples");

		result.isValid = result.gameUpdateVideo && result.gameUpdateAudio;
	}

	if (!result.isValid)
	{
		result.gameUpdateVideo = GameUpdateAndRenderStub;
		result.gameUpdateAudio = GameGetSoundSamplesStub;
	}

	return result;
}

internal_function void Win32UnloadGameCode(win32_game_code* game)
{
	if (game->gameDLL)
	{
		FreeLibrary(game->gameDLL);
		game->gameDLL = 0;
	}

	game->gameDLL = (HMODULE) false;
	game->gameUpdateVideo = GameUpdateAndRenderStub;
	game->gameUpdateAudio = GameGetSoundSamplesStub;
}

internal_function void Win32LoadXInput()
{
	HMODULE library = LoadLibraryA("xinput1_4.dll");
	if (library)
	{
		xInputGetState_ = (x_input_get_state*)GetProcAddress(library, "XInputGetState");
		xInputSetState_ = (x_input_set_state*)GetProcAddress(library, "XInputSetState");
	}
	else
	{
		// log diagnostic
	}
}

internal_function void Win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
	HMODULE library = LoadLibraryA("dsound.dll");
	if (library)
	{
		direct_sound_create* directSoundCreate = (direct_sound_create*)GetProcAddress(library, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if (SUCCEEDED(directSoundCreate(0, &directSound, 0)))
		{
			if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
				// success
			}

			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = samplesPerSecond;
			waveFormat.wBitsPerSample = 16;
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

			DSBUFFERDESC bufferDescription = {};
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
			LPDIRECTSOUNDBUFFER primaryBuffer;
			if (SUCCEEDED(
				directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
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
			
			bufferDescription.dwSize = sizeof(bufferDescription);
			bufferDescription.dwFlags = 0;
			bufferDescription.dwBufferBytes = bufferSize;
			bufferDescription.lpwfxFormat = &waveFormat;
			if (SUCCEEDED(
				directSound->CreateSoundBuffer(&bufferDescription, &g_sound_buffer, 0)))
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

	if (SUCCEEDED(g_sound_buffer->Lock(
		0, soundOutput->bufferSize,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		uint8* out = (uint8*)Region1;
		for (DWORD byteIndex = 0; byteIndex < Region1Size; ++byteIndex)
		{
			*out++ = 0;
		}

		out = (uint8*)Region2;

		for (DWORD byteIndex = 0; byteIndex < Region2Size; ++byteIndex)
		{
			*out++ = 0;
		}

		g_sound_buffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal_function void Win32FillSoundBuffer(
	win32_sound_output* soundOutput,
	game_sound_output_buffer* sourceBuffer,
	DWORD lockOffset,
	DWORD byteToLock)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(g_sound_buffer->Lock(
		lockOffset, byteToLock,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		int16* dest = (int16*)Region1;
		int16* src = sourceBuffer->samples;
		DWORD region1SampleCount = Region1Size / soundOutput->bytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < region1SampleCount; ++SampleIndex)
		{
			*dest++ = *src++;
			*dest++ = *src++;
			soundOutput->runningSampleIndex++;
		}

		dest = (int16*)Region2;
		DWORD region2SampleCount = Region2Size / soundOutput->bytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < region2SampleCount; ++SampleIndex)
		{
			*dest++ = *src++;
			*dest++ = *src++;
			soundOutput->runningSampleIndex++;
		}

		g_sound_buffer->Unlock(
			Region1, Region1Size,
			Region2, Region2Size);
	}
}

internal_function void Win32ProcessKeyboardButtonState(
	game_button_state* newState,
	bool32 isDown)
{
	if (newState->endedDown != isDown) {
		newState->endedDown = isDown;
		newState->halfTransitionCount++;
	}
}

internal_function void Win32ProcessXInputButtonState(
	game_button_state* oldState,
	game_button_state* newState,
	DWORD buttons,
	DWORD buttonBit)
{
	newState->halfTransitionCount
		= oldState->endedDown == newState->endedDown ? 0 : 1;
	newState->endedDown = buttons & buttonBit;
}

internal_function real32 Win32ProcessXInputStickValue(SHORT thumbStick,
	SHORT deadzone)
{
	real32 result = 0;
	real32 max = 0;
	if (thumbStick < -deadzone)
	{
		max = 32768.0f;
	}
	else if (thumbStick > deadzone)
	{
		max = 32767.0f;
	}
	if (max != 0) {
		result = (real32)thumbStick / max;
	}

	return result;
}

internal_function void Win32ProcessMessages(game_controller_input* keyboardController)
{
	MSG message = {};

	while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			uint32 VKCode = (uint32)message.wParam;
			bool32 wasDown = (message.lParam & (1 << 30)) != 0;
			bool32 isDown = (message.lParam & (1 << 31)) == 0;

			if (isDown)
			{
				bool32 isAltDown = message.lParam & (1 << 29);

				if (VKCode == VK_F4 && isAltDown) {
					g_running = false;
				}

				if (VKCode == VK_RETURN && isAltDown)
				{
					//ToggleFullscreen(message.hwnd);
				}
			}

			if (wasDown != isDown) {
				switch (VKCode) {
				case 'W': {
					Win32ProcessKeyboardButtonState(&keyboardController->moveUp,
						isDown);
					break;
				}
				case 'A': {
					Win32ProcessKeyboardButtonState(&keyboardController->moveLeft,
						isDown);
					break;
				}
				case 'S': {
					Win32ProcessKeyboardButtonState(&keyboardController->moveDown,
						isDown);
					break;
				}
				case 'D': {
					Win32ProcessKeyboardButtonState(&keyboardController->moveRight,
						isDown);
					break;
				}
				case 'P': {
					if (message.message == WM_KEYDOWN) {
						g_debug_pause_render = !g_debug_pause_render;
					}
				};

				case VK_UP: {
					Win32ProcessKeyboardButtonState(&keyboardController->actUp,
						isDown);
					break;
				}

				case VK_DOWN: {
					Win32ProcessKeyboardButtonState(&keyboardController->actDown,
						isDown);
					break;
				}

				case VK_LEFT: {
					Win32ProcessKeyboardButtonState(&keyboardController->actLeft,
						isDown);
					break;
				}

				case VK_RIGHT: {
					Win32ProcessKeyboardButtonState(&keyboardController->actRight,
						isDown);
					break;
				}

				case VK_ESCAPE: {
					Win32ProcessKeyboardButtonState(&keyboardController->back,
						isDown);
					break;
				}

				case VK_SPACE: {
					Win32ProcessKeyboardButtonState(&keyboardController->start,
						isDown);
					break;
				}

							 // left shoulder
				case 'Q': {
					Win32ProcessKeyboardButtonState(&keyboardController->leftShoulder,
						isDown);
					break;
				}

						// right shoulder
				case 'E': {
					Win32ProcessKeyboardButtonState(
						&keyboardController->rightShoulder,
						isDown);
					break;
				}
				}
			}
			break;
		}

		default:
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
			break;
		}
		}
	}
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
		break;
	}
	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
		break;
	}
	case WM_DESTROY:
	{
		g_running = false;
		OutputDebugStringA("WM_DESTROY\n");
		break;
	}

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{ break; }

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
		break;
	}

	default:
	{
		result = DefWindowProcA(Window, Message, WParam, LParam);
		break;
	}
	}

	return(result);
}

int main(HINSTANCE Instance) {
	win32_game_code game = Win32LoadGameCode();

	LARGE_INTEGER perfCountFreqResult;
	QueryPerformanceFrequency(&perfCountFreqResult);
	g_perf_count_freq = perfCountFreqResult.QuadPart;

	UINT desiredSchedulerMS = 1;
	//bool32 sleepIsGranular = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;//linker breaks if above used
	bool32 sleepIsGranular = true;

	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	Win32ResizeDIBSection(&g_back_buffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "BareBonesWindowClass";

	int monitorRefreshHz = 60;
	int gameUpdateHz = monitorRefreshHz / 2;
	real32 targetSecondsPerFrame = 1.0f / (real32)gameUpdateHz;

	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Bare Bones",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			0, 0, Instance, 0);
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

			g_running = true;
			g_sound_buffer->Play(0, 0, DSBPLAY_LOOPING);
			int16* samples = (int16*)VirtualAlloc(0, soundOutput.bufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if BAREBONES_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
			LPVOID BaseAddress = 0;
#endif
			game_memory gameMemory = {};
			gameMemory.permStorageSize = Megabytes(64);
			gameMemory.tempStorageSize = Gigabytes(1);

			uint64 totalSize = gameMemory.permStorageSize + gameMemory.tempStorageSize;
			gameMemory.permStorage = VirtualAlloc(BaseAddress, totalSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			gameMemory.tempStorage = ((uint8*)gameMemory.tempStorageSize +
				gameMemory.permStorageSize);

			if (samples && gameMemory.permStorage && gameMemory.tempStorage)
			{
				game_input gameInput[2] = {};
				game_input* newInput = &gameInput[0];
				game_input* oldInput = &gameInput[1];

				LARGE_INTEGER lastCounter = Win32GetWallClock();

				win32_game_code game = Win32LoadGameCode();
				uint32 loadCounter = 120;

				uint64 lastCycleCount = __rdtsc();
				while (g_running)
				{
					if (loadCounter++ > 120)
					{
						Win32UnloadGameCode(&game);
						game = Win32LoadGameCode();
						loadCounter = 0;
					}

					game_controller_input* oldKeyboardController = &oldInput->controllers[0];
					game_controller_input* newKeyboardController = &newInput->controllers[0];
					*newKeyboardController = {};
					newKeyboardController->isConnected = true;
					for (int buttonIndex = 0;
						buttonIndex < ArrayCount(newKeyboardController->buttons);
						++buttonIndex)
					{
						newKeyboardController->buttons[buttonIndex].endedDown =
							oldKeyboardController->buttons[buttonIndex].endedDown;
					}

					Win32ProcessMessages(newKeyboardController);

					// Controller
					DWORD maxControllerCount = XUSER_MAX_COUNT;
					if (maxControllerCount > ArrayCount(newInput->controllers) - 1)
					{
						maxControllerCount = ArrayCount(newInput->controllers) - 1;
					}
					for (DWORD ctrlIndex = 0; ctrlIndex < maxControllerCount; ++ctrlIndex)
					{
						DWORD curCtrlIdx = ctrlIndex + 1;
						game_controller_input* oldController =
							&oldInput->controllers[curCtrlIdx];
						game_controller_input* newController =
							&newInput->controllers[curCtrlIdx];

						XINPUT_STATE state;
						DWORD inputState = XInputGetState(ctrlIndex, &state);

						// Controller Mapping
						if (inputState == ERROR_SUCCESS)
						{
							// This controller is plugged in
							newController->isConnected = true;

							XINPUT_GAMEPAD* pad = &state.Gamepad;

							newController->isAnalog = true;
							newController->stickAverageX =
								Win32ProcessXInputStickValue(pad->sThumbLX,
									XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							newController->stickAverageY =
								Win32ProcessXInputStickValue(pad->sThumbLY,
									XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

							//int16 stickRX = (real32)pad->sThumbRX;
							//int16 stickRY = (real32)pad->sThumbRY;

							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
							{
								newController->stickAverageY = 1.0f;
							}
							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
							{
								newController->stickAverageY = -1.0f;
							}
							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
							{
								newController->stickAverageX = -1.0f;
							}
							if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
							{
								newController->stickAverageX = 1.0f;
							}

							real32 deadzone = 0.5f;
							Win32ProcessXInputButtonState(
								&oldController->moveLeft,
								&newController->moveLeft,
								(newController->stickAverageX < -deadzone ? 1 : 0),
								1);
							Win32ProcessXInputButtonState(
								&oldController->moveRight,
								&newController->moveRight,
								(newController->stickAverageX > deadzone ? 1 : 0),
								1);

							Win32ProcessXInputButtonState(
								&oldController->moveDown,
								&newController->moveDown,
								(newController->stickAverageY < -deadzone ? 1 : 0),
								1);

							Win32ProcessXInputButtonState(
								&oldController->moveUp,
								&newController->moveUp,
								(newController->stickAverageY > deadzone ? 1 : 0),
								1);

							Win32ProcessXInputButtonState(
								&oldController->actDown, &newController->actDown,
								pad->wButtons, XINPUT_GAMEPAD_A);
							Win32ProcessXInputButtonState(
								&oldController->actRight, &newController->actRight,
								pad->wButtons, XINPUT_GAMEPAD_B);
							Win32ProcessXInputButtonState(
								&oldController->actLeft, &newController->actLeft,
								pad->wButtons, XINPUT_GAMEPAD_X);
							Win32ProcessXInputButtonState(
								&oldController->actUp, &newController->actUp,
								pad->wButtons, XINPUT_GAMEPAD_Y);

							Win32ProcessXInputButtonState(
								&oldController->leftShoulder, &newController->leftShoulder,
								pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
							Win32ProcessXInputButtonState(
								&oldController->rightShoulder, &newController->rightShoulder,
								pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
							Win32ProcessXInputButtonState(
								&oldController->start, &newController->start,
								pad->wButtons, XINPUT_GAMEPAD_START);
							Win32ProcessXInputButtonState(
								&oldController->back, &newController->back,
								pad->wButtons, XINPUT_GAMEPAD_BACK);

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
							newController->isConnected = false;
						}
					}

					game_offscreen_buffer buffer = {};
					buffer.memory = g_back_buffer.memory;
					buffer.width = g_back_buffer.width;
					buffer.height = g_back_buffer.height;
					buffer.pitch = g_back_buffer.pitch;
					game.gameUpdateVideo(&gameMemory, newInput, &buffer);

					LARGE_INTEGER workCounter = Win32GetWallClock();
					real32 workSecondsElapsed = Win32GetSecondsElapsed(
						lastCounter, workCounter);

					DWORD byteToLock = 0;
					DWORD targetCursor = 0;
					DWORD bytesToWrite = 0;
					DWORD playCursor = 0;
					DWORD writeCursor = 0;
					bool32 soundIsValid = false;
					if (SUCCEEDED(g_sound_buffer->GetCurrentPosition(&playCursor, &writeCursor)))
					{
						byteToLock = ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample)
							% soundOutput.bufferSize);
						targetCursor = (playCursor +
							(soundOutput.latencySample * soundOutput.bytesPerSample)) %
							soundOutput.bufferSize;

						if (byteToLock > targetCursor)
						{
							bytesToWrite = (soundOutput.bufferSize - byteToLock);
							bytesToWrite += targetCursor;
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
					game.gameUpdateAudio(&gameMemory, &soundBuffer);

					if (soundIsValid)
					{
						Win32FillSoundBuffer(&soundOutput, &soundBuffer, byteToLock, bytesToWrite);
					}

					real32 secondsElapsedPerFrame = workSecondsElapsed;
					if (secondsElapsedPerFrame > targetSecondsPerFrame)
					{
						//missed frame rate
					}
					if (sleepIsGranular)
					{
						DWORD sleepMS = (DWORD)(1000.0f * (targetSecondsPerFrame - secondsElapsedPerFrame));
						if (sleepMS > 0)
						{
							Sleep(sleepMS);
						}
					}
					while (secondsElapsedPerFrame <= targetSecondsPerFrame)
					{
						secondsElapsedPerFrame = Win32GetSecondsElapsed(
							lastCounter, Win32GetWallClock());
					}
					//for testing
					g_sound_buffer->GetCurrentPosition(&playCursor, &writeCursor);

					win32_window_dimensions dimensions = Win32GetWindowDimensions(Window);
					Win32DisplayBufferToWindow(
						&g_back_buffer, DeviceContext,
						dimensions.width, dimensions.height);

					//Swap(oldInput, newInput);
					game_input* temp = newInput;
					newInput = oldInput;
					oldInput = temp;

					LARGE_INTEGER endCounter = Win32GetWallClock();
					real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(lastCounter, endCounter);
					lastCounter = endCounter;

					uint64 endCycleCount = __rdtsc();
					uint64 cyclesElapsed = endCycleCount - lastCycleCount;
					lastCycleCount = endCycleCount;

					//real32 fps = (real32)g_perf_count_frequency / (real32)counterElapsed;
					real32 MCPF = (real32)cyclesElapsed / 1000000.0f;
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