//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

template<class T, class U, HWND (U::*m_hWnd)> T*
InstanceFromWndProc(HWND hWnd, UINT uMsg, LPARAM lParam)
{
    // Get the pointer to the class instance.
    T* pInstance;
    if (uMsg == WM_NCCREATE)
    {
        // The pointer has been passed via CreateWindowExW and now needs to be saved via SetWindowLongPtrW.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pInstance = reinterpret_cast<T*>(pCreateStruct->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pInstance));

        // We are handling the first useful window message, so this is the perfect place to also set the hWnd member variable.
        pInstance->*m_hWnd = hWnd;
    }
    else
    {
        // Get the pointer saved via SetWindowLongPtrW above.
        pInstance = reinterpret_cast<T*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    return pInstance;
}

// utils.cpp
void ErrorBox(const std::wstring& wstrMessage);
WORD GetWindowDPI(HWND hWnd);
std::wstring LoadStringAsWstr(HINSTANCE hInstance, UINT uID);
std::unique_ptr<Gdiplus::Bitmap> LoadPNGAsGdiplusBitmap(HINSTANCE hInstance, UINT uID);
HMODULE SafeLoadSystemLibrary(const std::wstring& LibraryName);
