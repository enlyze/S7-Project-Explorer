//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include "S7-Project-Explorer.h"

#define IDC_FILE        500
#define IDC_BROWSE      501


void
CFilePage::_OnBrowseButton()
{
    std::wstring wstrSelectedFilePath = std::wstring(MAX_PATH, L'\0');

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_pMainWindow->GetHwnd();
    ofn.lpstrFilter = m_wstrBrowseFilter.c_str();
    ofn.lpstrFile = wstrSelectedFilePath.data();
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = m_wstrBrowseTitle.c_str();
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
    {
        // Copy the newly selected file path into the member variable.
        m_wstrSelectedFilePath.assign(wstrSelectedFilePath, 0, wstrSelectedFilePath.find(L'\0'));

        // Set the file static control to the s7p file bitmap.
        m_pCurrentFileBitmap = m_pS7PFileBitmap.get();
        RedrawWindow(m_hFile, nullptr, nullptr, RDW_INVALIDATE);

        // Set the file name.
        size_t BackslashPosition = m_wstrSelectedFilePath.find_last_of(L'\\');
        PCWSTR pwszFileName = m_wstrSelectedFilePath.c_str() + BackslashPosition + 1;
        SetWindowTextW(m_hFileName, pwszFileName);

        // Enable the "Next" button.
        m_pMainWindow->EnableNextButton(TRUE);
    }
}

LRESULT
CFilePage::_OnCommand(WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        // Open the Browse dialog when the user clicks on the File icon.
        case IDC_FILE:
        {
            if (HIWORD(wParam) == STN_CLICKED)
            {
                _OnBrowseButton();
            }

            break;
        }

        // Also open the Browse dialog when the user clicks on the "Browse" button.
        case IDC_BROWSE:
        {
            _OnBrowseButton();
            break;
        }
    }

    return 0;
}

LRESULT
CFilePage::_OnCreate()
{
    // Load resources.
    m_pBlankFileBitmap = LoadPNGAsGdiplusBitmap(m_pMainWindow->GetHInstance(), IDP_BLANK_FILE);
    m_pS7PFileBitmap = LoadPNGAsGdiplusBitmap(m_pMainWindow->GetHInstance(), IDP_S7P_FILE);
    m_wstrBrowseTitle = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_BROWSE_TITLE);
    m_wstrHeader = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_FILEPAGE_HEADER);
    m_wstrSubHeader = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_FILEPAGE_SUBHEADER);
    m_wstrText = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_FILEPAGE_TEXT);
    m_wstrNext = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_NEXT);

    // Load the browse filter and replace the '|' characters in the resource string by NUL-characters to create the required double-NUL-terminated string.
    // This is needed, because you cannot reliably store double-NUL-terminated strings in resources.
    m_wstrBrowseFilter = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_BROWSE_FILTER);
    std::replace(m_wstrBrowseFilter.begin(), m_wstrBrowseFilter.end(), L'|', L'\0');

    // Create controls.
    m_hFile = CreateWindowExW(0, WC_STATICW, L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_NOTIFY, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_FILE), nullptr, nullptr);

    m_hFileName = CreateWindowExW(0, WC_STATICW, L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 0, 0, 0, m_hWnd, nullptr, nullptr, nullptr);
    SendMessageW(m_hFileName, WM_SETFONT, reinterpret_cast<WPARAM>(m_pMainWindow->GetGuiFont()), MAKELPARAM(TRUE, 0));

    std::wstring wstrBrowse = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_BROWSE);
    m_hFileBrowse = CreateWindowExW(0, WC_BUTTONW, wstrBrowse.c_str(), WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_BROWSE), nullptr, nullptr);
    SendMessageW(m_hFileBrowse, WM_SETFONT, reinterpret_cast<WPARAM>(m_pMainWindow->GetGuiFont()), MAKELPARAM(TRUE, 0));

    // Set the file static control to the blank file bitmap for now.
    m_pCurrentFileBitmap = m_pBlankFileBitmap.get();

    return 0;
}

LRESULT
CFilePage::_OnDrawItem(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // We only draw the file icon.
    if (wParam != IDC_FILE)
    {
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    // We only need to draw when the entire control needs to be drawn (not on focus or selection changes).
    PDRAWITEMSTRUCT pDis = reinterpret_cast<PDRAWITEMSTRUCT>(lParam);
    if (pDis->itemAction != ODA_DRAWENTIRE)
    {
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    // Draw the file bitmap using anti-aliased GDI+ drawing.
    Gdiplus::Graphics g(pDis->hDC);
    Gdiplus::Color BackgroundColor;
    BackgroundColor.SetFromCOLORREF(GetSysColor(COLOR_BTNFACE));
    g.Clear(BackgroundColor);
    g.DrawImage(m_pCurrentFileBitmap, 0, 0, pDis->rcItem.right, pDis->rcItem.bottom);
    
    return TRUE;
}

LRESULT
CFilePage::_OnEraseBkgnd()
{
    // We always repaint the entire window in _OnPaint.
    // Therefore, don't forward WM_ERASEBKGND messages to DefWindowProcW to prevent flickering.
    return TRUE;
}

LRESULT
CFilePage::_OnPaint()
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

    // Draw the intro text.
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

LRESULT
CFilePage::_OnSetCursor(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Set the hand cursor when the mouse moves over the file image.
    if (reinterpret_cast<HWND>(wParam) == m_hFile)
    {
        HCURSOR hHandCursor = LoadCursorW(nullptr, IDC_HAND);
        SetCursor(hHandCursor);
        return TRUE;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT
CFilePage::_OnSize()
{
    // Get the window size.
    RECT rcWindow;
    GetClientRect(m_hWnd, &rcWindow);

    // Invalidate all areas with text.
    InvalidateRect(m_hWnd, &rcWindow, FALSE);
    InvalidateRect(m_hFileName, nullptr, FALSE);

    // Move the file icon.
    HDWP hDwp = BeginDeferWindowPos(3);
    if (!hDwp)
    {
        return 0;
    }

    int iFileHeight = m_pMainWindow->ScaleControl(140);
    int iFileWidth = m_pMainWindow->ScaleControl(105);
    int iFileX = (rcWindow.right - iFileWidth) / 2;
    int iFileY = (rcWindow.bottom - iFileHeight) / 2;
    hDwp = DeferWindowPos(hDwp, m_hFile, nullptr, iFileX, iFileY, iFileWidth, iFileHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    // Move the file name label.
    int iFileNameHeight = m_pMainWindow->ScaleFont(20);
    int iFileNameWidth = rcWindow.right;
    int iFileNameX = 0;
    int iFileNameY = iFileY + iFileHeight;
    hDwp = DeferWindowPos(hDwp, m_hFileName, nullptr, iFileNameX, iFileNameY, iFileNameWidth, iFileNameHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    // Move the Browse button.
    int iBrowseHeight = m_pMainWindow->ScaleControl(30);
    int iBrowseWidth = m_pMainWindow->ScaleControl(105);
    int iBrowseX = (rcWindow.right - iBrowseWidth) / 2;
    int iBrowseY = iFileNameY + iFileNameHeight;
    hDwp = DeferWindowPos(hDwp, m_hFileBrowse, nullptr, iBrowseX, iBrowseY, iBrowseWidth, iBrowseHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    EndDeferWindowPos(hDwp);

    return 0;
}

LRESULT CALLBACK
CFilePage::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CFilePage* pPage = InstanceFromWndProc<CFilePage, CPage, &CFilePage::CPage::m_hWnd>(hWnd, uMsg, lParam);

    if (pPage)
    {
        switch (uMsg)
        {
            case WM_COMMAND: return pPage->_OnCommand(wParam);
            case WM_CREATE: return pPage->_OnCreate();
            case WM_DRAWITEM: return pPage->_OnDrawItem(hWnd, uMsg, wParam, lParam);
            case WM_ERASEBKGND: return pPage->_OnEraseBkgnd();
            case WM_PAINT: return pPage->_OnPaint();
            case WM_SETCURSOR: return pPage->_OnSetCursor(hWnd, uMsg, wParam, lParam);
            case WM_SIZE: return pPage->_OnSize();
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void
CFilePage::SwitchTo()
{
    m_pMainWindow->SetHeader(&m_wstrHeader, &m_wstrSubHeader);
    m_pMainWindow->EnableBackButton(FALSE);
    m_pMainWindow->SetNextButtonText(m_wstrNext);
    m_pMainWindow->ShowWarningsButton(FALSE);
    ShowWindow(m_hWnd, SW_SHOW);
}

void
CFilePage::UpdateDPI()
{ 
    // Update the control fonts.
    SendMessageW(m_hFileName, WM_SETFONT, reinterpret_cast<WPARAM>(m_pMainWindow->GetGuiFont()), MAKELPARAM(TRUE, 0));
    SendMessageW(m_hFileBrowse, WM_SETFONT, reinterpret_cast<WPARAM>(m_pMainWindow->GetGuiFont()), MAKELPARAM(TRUE, 0));
}
