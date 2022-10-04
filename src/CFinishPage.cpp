//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include "S7-Project-Explorer.h"

LRESULT
CFinishPage::_OnCreate()
{
    // Load resources.
    m_wstrHeader = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_FINISHPAGE_HEADER);
    m_wstrSubHeader = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_FINISHPAGE_SUBHEADER);
    m_wstrText = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_FINISHPAGE_TEXT);
    m_wstrFinish = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_FINISH);

    return 0;
}

LRESULT
CFinishPage::_OnEraseBkgnd()
{
    // We always repaint the entire window in _OnPaint.
    // Therefore, don't forward WM_ERASEBKGND messages to DefWindowProcW to prevent flickering.
    return TRUE;
}

LRESULT
CFinishPage::_OnPaint()
{
    // Get the window size.
    RECT rcWindow;
    GetClientRect(m_hWnd, &rcWindow);

    // Begin a double-buffered paint.
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(m_hWnd, &ps);
    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hMemBitmap = CreateCompatibleBitmap(hDC, rcWindow.right, rcWindow.bottom);
    SelectObject(hMemDC, hMemBitmap);

    // Fill the window with the window background color.
    FillRect(hMemDC, &rcWindow, GetSysColorBrush(COLOR_BTNFACE));

    // Draw the finish text.
    SelectObject(hMemDC, m_pMainWindow->GetGuiFont());
    SetBkColor(hMemDC, GetSysColor(COLOR_BTNFACE));
    DrawTextW(hMemDC, m_wstrText.c_str(), m_wstrText.size(), &rcWindow, DT_WORDBREAK);

    // End painting by copying the in-memory DC.
    BitBlt(hDC, 0, 0, rcWindow.right, rcWindow.bottom, hMemDC, 0, 0, SRCCOPY);
    DeleteObject(hMemBitmap);
    DeleteDC(hMemDC);
    EndPaint(m_hWnd, &ps);

    return 0;
}

LRESULT CALLBACK
CFinishPage::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CFinishPage* pPage = InstanceFromWndProc<CFinishPage, CPage, &CFinishPage::CPage::m_hWnd>(hWnd, uMsg, lParam);

    if (pPage)
    {
        switch (uMsg)
        {
            case WM_CREATE: return pPage->_OnCreate();
            case WM_ERASEBKGND: return pPage->_OnEraseBkgnd();
            case WM_PAINT: return pPage->_OnPaint();
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void
CFinishPage::SwitchTo()
{
    m_pMainWindow->SetHeader(&m_wstrHeader, &m_wstrSubHeader);
    m_pMainWindow->EnableBackButton(FALSE);
    m_pMainWindow->EnableCancelButton(FALSE);
    m_pMainWindow->SetNextButtonText(m_wstrFinish);
    m_pMainWindow->ShowWarningsButton(FALSE);
    ShowWindow(m_hWnd, SW_SHOW);
}

void
CFinishPage::UpdateDPI()
{
}
