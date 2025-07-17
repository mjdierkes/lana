/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


#include "stdhdrs.h"
#include "MouseSimulator.h"
#include "resource.h"
HINSTANCE CursorColorManager::hInst = NULL;
CursorColorManager* CursorColorManager::instance = NULL;

CursorColorManager::CursorColorManager()
{
    
}

CursorColorManager::~CursorColorManager()
{
    for (int i = 1; i < 9; i++)
        DestroyCursor(hCorsor[i]);
}

void CursorColorManager::Init(HINSTANCE hInst)
{ 
    if (!this->hInst) {
        this->hInst = hInst;
        hCorsor[0] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSORBLANK));
        hCorsor[1] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR1));
        hCorsor[2] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR2));
        hCorsor[3] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR3));
        hCorsor[4] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR4));
        hCorsor[5] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR6));
        hCorsor[6] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR7));
        //hCorsor[7] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR8));
        //hCorsor[8] = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR9));
        for (int i = 0; i < 7; i++)
            usedCursor[i] = false;
    }
}

HICON CursorColorManager::getCursor()
{
    for (int i = 1; i < 7; i++)
        if (usedCursor[i] == false) {
            usedCursor[i] = true;
            return hCorsor[i];
        }
    return NULL;
}

void CursorColorManager::releaseCursor(HICON icon)
{
    for (int i = 1; i < 7; i++)
        if (hCorsor[i] == icon)
            usedCursor[i] = false;
}

HICON CursorColorManager::getEraser()
{
    return hCorsor[0];
}

CursorColorManager* CursorColorManager::getInstance()
{
    if (instance == 0)
    {
        instance = new CursorColorManager();
    }

    return instance;
}

SimulateCursor::SimulateCursor(HINSTANCE hInst)
{
    this->hInst = hInst;
    CursorColorManager::getInstance()->Init((HINSTANCE)hInst);
    hIconMouse = CursorColorManager::getInstance()->getCursor();
    hIconErase = CursorColorManager::getInstance()->getEraser();
    DWORD dwTId;
    CreateThread(NULL, 0, Start, this, 0, &dwTId);
}

SimulateCursor:: ~SimulateCursor()
{
    SendMessage(hWnd, WM_CLOSE, 0, 0);
    CursorColorManager::getInstance()->releaseCursor(hIconMouse);
}

DWORD WINAPI SimulateCursor::Start(LPVOID lpParam)
{
    SimulateCursor* sc = (SimulateCursor*)lpParam;
    HDESK desktop;
    desktop = OpenInputDesktop(0, FALSE,
        DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
        DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
        DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
        DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
    );
    HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
    if (desktop)
    {
        SetThreadDesktop(desktop);
    }

    sc->hWnd = create_window(sc);
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SetThreadDesktop(old_desktop);
    if (desktop)
        CloseDesktop(desktop);

    return 0;
}

HWND SimulateCursor::create_window(SimulateCursor* simulateCursor)
{
    SimulateCursor* sc = simulateCursor;
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = (HINSTANCE)sc->hInst;
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    wcex.lpszMenuName = 0;;
    wcex.lpszClassName = "mouseSimulator";
    wcex.hIconSm = NULL;
    RegisterClassEx(&wcex);
    HWND hWndDesktop = GetDesktopWindow();
    RECT rcClient;
    if (!GetClientRect(hWndDesktop, &rcClient))
        return 0;

    HWND hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "mouseSimulator",
        "mouseSimulator",
        WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
        rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
        hWndDesktop,
        NULL,
        (HINSTANCE)sc->hInst,
        nullptr);

#if !defined( _WIN64 )
    SetWindowLong(hWnd, GWL_USERDATA, (long)sc);
#else
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)sc);
#endif

#ifndef _X64
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(hWnd, GWL_STYLE, style);
#else
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    style = GetWindowLongPtr(hWnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLongPtr(hWnd, GWL_STYLE, style);
#endif

#ifndef _X64
    LONG lExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLong(hWnd, GWL_EXSTYLE, lExStyle);
#else
    LONG_PTR lExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLongPtr(hWnd, GWL_EXSTYLE, lExStyle);
#endif

    ShowWindow(hWnd, true);
    UpdateWindow(hWnd);
    SetLayeredWindowAttributes(hWnd, RGB(255, 255, 255), 0, LWA_COLORKEY);
    return hWnd;
}

void SimulateCursor::moveCursor(int x, int y)
{
    /*this->x = x;
    this->y = y;
    RECT rect;
    rect.left = x; rect.top = y;
    rect.right = x + 32; rect.bottom = y + 32;
    RECT rect2;
    rect2.left = oldx; rect2.top = oldy;
    rect2.right = oldx + 32; rect2.bottom = oldy + 32;
    HRGN rgn = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);*/

    InvalidateRect(hWnd, NULL, true);
}

LRESULT CALLBACK SimulateCursor::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    SimulateCursor* me;
#if !defined( _WIN64 )
    me = reinterpret_cast<SimulateCursor*>(GetWindowLong(hWnd, GWL_USERDATA));
#else
    me = reinterpret_cast<SimulateCursor*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
#endif
    if (me)
        return me->realWndProc(hWnd, message, wParam, lParam);
    return DefWindowProc(hWnd, message, wParam, lParam);
}



LRESULT CALLBACK SimulateCursor::realWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (drawn && (x != oldx || y != oldy))
            DrawIcon(hdc, oldx, oldy, hIconErase);
        DrawIcon(hdc, x, y, hIconMouse);
        oldx = x, oldy = y;
        drawn = true;
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}