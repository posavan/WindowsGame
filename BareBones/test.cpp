#include <Windows.h>

int main(void) // function signature
{
	OutputDebugStringA("This is the first to be printed.\n");
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd)
{
	main();
}
