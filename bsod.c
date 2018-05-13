/* Creates a fake Blue Screen Of Death which looks like the win7 version of it*/
/*
 * bsod.c
 * Copyright (C) 2017 Zhang Maiyun <myzhang1029@163.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <Windows.h>
#include <stdio.h>
#include <time.h>
#ifndef __GNUC__
#define __UNUSED
#else
#define __UNUSED __attribute__((unused))
#endif
int SetUpWindowClass(char *, int, int, int, HINSTANCE);
LRESULT CALLBACK WindowProcedure(HWND, unsigned int, WPARAM, LPARAM);
unsigned redrawcount;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance __UNUSED, LPSTR lpCmdLine __UNUSED,
		   int nCmdShow __UNUSED)
{
	HWND hwnd;
	MSG uMsg;
	if (!SetUpWindowClass("1", 0, 0, 128, hInstance)) /* Navy blue */
		return 1;
	redrawcount = 0;
	hwnd = CreateWindow("1", 0, WS_BORDER, 0, 0, 100, 100, NULL, NULL, hInstance, NULL);
	SetWindowLongPtr(hwnd, GWL_STYLE, 0);
	if (!hwnd)
		return 1;

	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	FreeConsole(); /* remove the console window */
	SetCursor(NULL);
	while (GetMessage(&uMsg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&uMsg);
		DispatchMessage(&uMsg);
	}
	return 0;
}

int SetUpWindowClass(char *Title, int bgRed, int bgGreen, int bgBlue, HINSTANCE hInst)
{
	WNDCLASSEX WindowClass;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = 0;
	WindowClass.lpszClassName = Title;
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpfnWndProc = WindowProcedure;
	WindowClass.hInstance = hInst;
	WindowClass.hCursor = NULL;
	WindowClass.hbrBackground = CreateSolidBrush(RGB(bgRed, bgGreen, bgBlue));
	WindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WindowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (RegisterClassEx(&WindowClass))
		return 1;
	else
		return 0;
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, unsigned int uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
		case WM_KEYDOWN:
			if (wParam != 0x51)
				break;
			/* Pass down if 'Q' is hit */
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_WINDOWPOSCHANGING:
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
			break;
		case WM_PAINT:
		{
			char *Text[] = {
			    "A problem has been detected and windows has been shut down to prevent damage",
			    "to your computer.",
			    "",
			    "A process or thread crucial to system operation has unexpectedly exited or been",
			    "terminated.",
			    "",
			    "If this is the first time you've seen this stop error screen,",
			    "restart your computer. If this screen appears again, follow these steps:",
			    "",
			    "Check to make sure any new hardware or software is properly installed.",
			    "If this is a new installation, ask your hardware or software manufacturer",
			    "for any Windows updates you might need.",
			    "",
			    "If problems continue, disable or remove any newly installed hardware",
			    "or software. Disable BIOS memory options such as caching or shadowing.",
			    "If you need to use Safe Mode to remove or disable components, restart",
			    "your computer, press F8 to select Advanced Startup "
			    "Options, and then",
			    "select Safe Mode.",
			    "",
			    "Technical information:",
			    "",
			    "*** STOP: 0x000000F4 (0x00000003,0x87839060,0x87839CCC,0x83E40020)",
			    "",
			    "",
			    "",
			    "Collecting data for crash dump ...",
			    "Initializing disk for crash dump ...",
			    "Beginning dump of physical memory.",
			    "Dumping physical memory to disk: 0",
			    NULL};
			int horipos = 5, horiinc;
			char dumpstatus[37] = {0};
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);
			RECT winrect;
			HFONT font;
			GetWindowRect(hWnd, &winrect);
			horiinc = winrect.bottom / 36;
			font = CreateFont(winrect.bottom / 48, winrect.right / 84, 0, 0, 0, 0, 0, 0, ANSI_CHARSET,
					  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
					  "consolas");
			SetTextColor(hDC, RGB(192, 192, 192));
			SetBkColor(hDC, RGB(0, 0, 128));
			SelectObject(hDC, font);
			for (int count = 0; Text[count] != NULL; ++count, horipos += horiinc)
			{
				TextOut(hDC, 5, horipos, Text[count], strlen(Text[count]));
			}
			if (redrawcount == 0)/* Draw a increasing process if it's the first draw */
				for (int count = 5; count <= 100; count += 5)
				{
					clock_t t = clock(); /* Delay a second */
					while (1)
						if ((int)((clock() - t) / CLOCKS_PER_SEC) >= 1)
							break;
					snprintf(dumpstatus, 37, "Dumping physical memory to disk: %d", count);
					TextOut(hDC, 5, horipos - horiinc, dumpstatus, strlen(dumpstatus));
				}
			else
				TextOut(hDC, 5, horipos - horiinc, "Dumping physical memory to disk: 100", 37);
			TextOut(hDC, 5, horipos, "Physical memory dump complete.", 30);
			TextOut(hDC, 5, horipos + horiinc,
				"Contact your system admin or technical support group for further assistance.", 76);
			DeleteObject(font);
			EndPaint(hWnd, &ps);
			break;
		}
		default:
			return DefWindowProc(hWnd, uiMsg, wParam, lParam);
	}
	return 0;
}
