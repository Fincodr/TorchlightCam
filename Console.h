#pragma once

// width and height is the size of console window, if you specify fname, 
// the output will also be written to this file. The file pointer is automatically closed 
// when you close the app

static HANDLE __hStdOut = NULL;

void CreateConsoleWindow(int width=80, int height=25);

void CreateConsoleWindow(int width, int height)
{
	if ( AllocConsole() )
	{
		// Disable Close button (doesn't hide it on Windows Vista/7)
		HWND hwnd = GetConsoleWindow();
		HMENU hmenu = GetSystemMenu (hwnd, FALSE);
		HINSTANCE hinstance = (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE);
		DeleteMenu (hmenu, 6, MF_BYPOSITION);
		//while (DeleteMenu (hmenu, 0, MF_BYPOSITION));
		// Set control handler if specifies
		SetConsoleTitle("Debug Window");
		__hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		HWND ConsoleWin = GetConsoleWindow();
		SetFocus(ConsoleWin);

		COORD co = {width,height};
		SetConsoleScreenBufferSize(__hStdOut, co);
	}
	else
	{
		DWORD dw = GetLastError(); 
		char* lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			lpMsgBuf,
			0, NULL );
		MessageBox(0, lpMsgBuf, "ERROR WHILE OPENING CONSOLE", 0);
		LocalFree(lpMsgBuf);

	}
}

// Use ConsoleOut like TRACE0, TRACE1, ... (The arguments are the same as printf)
int ConsoleOut(char *fmt, ...)
{
	DWORD cCharsWritten;

	char s[1024];
	va_list argptr;
	int cnt;

	va_start(argptr, fmt);
	cnt = vsprintf(s, fmt, argptr);
	va_end(argptr);

	if(__hStdOut)
		WriteConsoleA(__hStdOut, s, strlen(s), &cCharsWritten, NULL);

	return(cnt);
}