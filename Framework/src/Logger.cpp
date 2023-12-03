#include <cstdio>
#include <cstdarg>
#include <Windows.h>

// output log
void OutputLog(const char* format, ...)
{
	char msg[2048];
	memset(msg, '\0', sizeof(msg));
	va_list arg;

	va_start(arg, format);
	vsprintf_s(msg, format, arg);
	va_end(arg);

	// output console
	printf_s("%s", msg);

	// output to visual studio output window
	OutputDebugStringA(msg);
}