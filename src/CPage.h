//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

class CPage
{
public:
    HWND GetHwnd() const { return m_hWnd; }
    virtual void SwitchTo() = 0;
    virtual void UpdateDPI() = 0;

protected:
    CMainWindow* m_pMainWindow;
    HWND m_hWnd;

    CPage(CMainWindow* pMainWindow) : m_pMainWindow(pMainWindow) {};

    template<class T> static std::unique_ptr<T>
    Create(CMainWindow* pMainWindow)
    {
        // Register the window class.
        WNDCLASSW wc = {};
        wc.lpfnWndProc = T::_WndProc;
        wc.hInstance = pMainWindow->GetHInstance();
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
        wc.lpszClassName = T::_wszWndClass;
        if (RegisterClassW(&wc) == 0)
        {
            ErrorBox(L"RegisterClassW failed, last error is " + std::to_wstring(GetLastError()));
            return nullptr;
        }

        // Create the page window.
        auto pPage = std::unique_ptr<T>(new T(pMainWindow));
        HWND hWnd = CreateWindowExW(
            0,
            T::_wszWndClass,
            L"",
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0,
            0,
            0,
            0,
            pMainWindow->GetHwnd(),
            nullptr,
            pMainWindow->GetHInstance(),
            pPage.get());
        if (hWnd == nullptr)
        {
            ErrorBox(L"CreateWindowExW failed, last error is " + std::to_wstring(GetLastError()));
            return nullptr;
        }

        return pPage;
    }
};
