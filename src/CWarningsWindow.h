//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

class CWarningsWindow
{
public:
    HWND GetHwnd() const { return m_hWnd; }

    static std::unique_ptr<CWarningsWindow> Create(CMainWindow* pMainWindow);
    void Open();

private:
    static constexpr WCHAR _wszWndClass[] = L"WarningsWndClass";

    sr::unique_resource<HFONT, decltype(DeleteObject)*> m_hGuiFont;
    CMainWindow* m_pMainWindow;
    HWND m_hClose;
    HWND m_hList;
    HWND m_hWnd;
    LOGFONTW m_lfGuiFont;
    WORD m_wCurrentDPI;

    CWarningsWindow(CMainWindow* pMainWindow);
    static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _OnCloseButton();
    LRESULT _OnCommand(WPARAM wParam);
    LRESULT _OnClose();
    LRESULT _OnCreate();
    LRESULT _OnDpiChanged(WPARAM wParam, LPARAM lParam);
    LRESULT _OnGetMinMaxInfo(LPARAM lParam);
    LRESULT _OnSize();
    int _ScaleControl(int iControlSize);
    int _ScaleFont(int iFontSize);
};
