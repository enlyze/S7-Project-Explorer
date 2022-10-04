//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include "S7-Project-Explorer.h"

static const int iMinWindowHeight = 400;
static const int iMinWindowWidth = 700;

#define IDC_CLOSE       500


CWarningsWindow::CWarningsWindow(CMainWindow* pMainWindow)
    : m_pMainWindow(pMainWindow)
{
}

LRESULT CALLBACK
CWarningsWindow::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWarningsWindow* pWarningsWindow = InstanceFromWndProc<CWarningsWindow, CWarningsWindow, &CWarningsWindow::m_hWnd>(hWnd, uMsg, lParam);

    // The first WM_GETMINMAXINFO comes before WM_NCCREATE, before we got our CMainWindow pointer.
    if (pWarningsWindow)
    {
        switch (uMsg)
        {
            case WM_COMMAND: return pWarningsWindow->_OnCommand(wParam);
            case WM_CLOSE: return pWarningsWindow->_OnClose();
            case WM_CREATE: return pWarningsWindow->_OnCreate();
            case WM_DPICHANGED: return pWarningsWindow->_OnDpiChanged(wParam, lParam);
            case WM_GETMINMAXINFO: return pWarningsWindow->_OnGetMinMaxInfo(lParam);
            case WM_SIZE: return pWarningsWindow->_OnSize();
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT
CWarningsWindow::_OnClose()
{
    EnableWindow(m_pMainWindow->GetHwnd(), TRUE);
    ShowWindow(m_hWnd, SW_HIDE);

    return 0;
}

LRESULT
CWarningsWindow::_OnCommand(WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_CLOSE: _OnClose(); break;
    }

    return 0;
}

LRESULT
CWarningsWindow::_OnCreate()
{
    // We are a separate window that may be on a monitor with a different DPI setting.
    m_wCurrentDPI = GetWindowDPI(m_hWnd);

    // Create the main GUI font.
    m_lfGuiFont = {};
    wcscpy_s(m_lfGuiFont.lfFaceName, L"MS Shell Dlg 2");
    m_lfGuiFont.lfHeight = -_ScaleFont(10);
    m_hGuiFont = make_unique_font(CreateFontIndirectW(&m_lfGuiFont));

    // Set up the ListView.
    m_hList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, 0, 0, 0, 0, m_hWnd, nullptr, nullptr, nullptr);
    ListView_SetExtendedListViewStyle(m_hList, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    LVCOLUMNW lvColumn = {};
    lvColumn.mask = LVCF_TEXT;

    std::wstring wstrColumn = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_DEVICE);
    lvColumn.pszText = wstrColumn.data();
    ListView_InsertColumn(m_hList, 0, &lvColumn);

    wstrColumn = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_WARNING);
    lvColumn.pszText = wstrColumn.data();
    ListView_InsertColumn(m_hList, 1, &lvColumn);

    // Create the Close button.
    std::wstring wstrClose = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_CLOSE);
    m_hClose = CreateWindowExW(0, WC_BUTTONW, wstrClose.c_str(), WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_CLOSE), nullptr, nullptr);
    SendMessageW(m_hClose, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));

    // Set the main window size.
    int iHeight = _ScaleControl(iMinWindowHeight);
    int iWidth = _ScaleControl(iMinWindowWidth);
    SetWindowPos(m_hWnd, nullptr, 0, 0, iWidth, iHeight, SWP_NOMOVE);

    return 0;
}

LRESULT
CWarningsWindow::_OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
    m_wCurrentDPI = LOWORD(wParam);

    // Recalculate the main GUI font.
    m_lfGuiFont.lfHeight = -_ScaleFont(10);
    m_hGuiFont = make_unique_font(CreateFontIndirectW(&m_lfGuiFont));

    // Update the control fonts.
    SendMessageW(m_hClose, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));

    // Use the suggested new window size.
    RECT* const prcNewWindow = reinterpret_cast<RECT*>(lParam);
    SetWindowPos(m_hWnd, nullptr, prcNewWindow->left, prcNewWindow->top, prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);

    return 0;
}

LRESULT
CWarningsWindow::_OnGetMinMaxInfo(LPARAM lParam)
{
    PMINMAXINFO pMinMaxInfo = reinterpret_cast<PMINMAXINFO>(lParam);

    pMinMaxInfo->ptMinTrackSize.x = _ScaleControl(iMinWindowWidth);
    pMinMaxInfo->ptMinTrackSize.y = _ScaleControl(iMinWindowHeight);

    return 0;
}

LRESULT
CWarningsWindow::_OnSize()
{
    // Get the window size.
    RECT rcWindow;
    GetClientRect(m_hWnd, &rcWindow);

    // Move the Close button.
    HDWP hDwp = BeginDeferWindowPos(2);
    if (!hDwp)
    {
        return 0;
    }

    const int iControlPadding = _ScaleControl(iUnifiedControlPadding);
    const int iButtonHeight = _ScaleControl(iUnifiedButtonHeight);
    const int iButtonWidth = _ScaleControl(iUnifiedButtonWidth);
    int iButtonX = (rcWindow.right - iButtonWidth) / 2;
    int iButtonY = rcWindow.bottom - iControlPadding - iButtonHeight;
    hDwp = DeferWindowPos(hDwp, m_hClose, nullptr, iButtonX, iButtonY, iButtonWidth, iButtonHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    // Move the ListView.
    int iListX = iControlPadding;
    int iListY = iControlPadding;
    int iListHeight = rcWindow.bottom - iControlPadding - iButtonHeight - iControlPadding - iListY;
    int iListWidth = rcWindow.right - iControlPadding - iListX;
    hDwp = DeferWindowPos(hDwp, m_hList, nullptr, iListX, iListY, iListWidth, iListHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    EndDeferWindowPos(hDwp);

    // Adjust the list column widths.
    LONG lColumnWidth = iListWidth / 2 - iControlPadding;
    ListView_SetColumnWidth(m_hList, 0, lColumnWidth);
    ListView_SetColumnWidth(m_hList, 1, lColumnWidth);

    return 0;
}

int
CWarningsWindow::_ScaleControl(int iControlSize)
{
    return MulDiv(iControlSize, m_wCurrentDPI, iWindowsReferenceDPI);
}

int
CWarningsWindow::_ScaleFont(int iFontSize)
{
    return MulDiv(iFontSize, m_wCurrentDPI, iFontReferenceDPI);
}

std::unique_ptr<CWarningsWindow>
CWarningsWindow::Create(CMainWindow* pMainWindow)
{
    // Register the warnings window class.
    WNDCLASSW wc = {};
    wc.lpfnWndProc = CWarningsWindow::_WndProc;
    wc.hInstance = pMainWindow->GetHInstance();
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(pMainWindow->GetHInstance(), MAKEINTRESOURCEW(IDI_ICON));
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = CWarningsWindow::_wszWndClass;

    if (RegisterClassW(&wc) == 0)
    {
        ErrorBox(L"RegisterClassW failed for CWarningsWindow, last error is " + std::to_wstring(GetLastError()));
        return nullptr;
    }

    // Create the warnings window.
    auto pWarningsWindow = std::unique_ptr<CWarningsWindow>(new CWarningsWindow(pMainWindow));
    HWND hWnd = CreateWindowExW(
        0,
        CWarningsWindow::_wszWndClass,
        L"",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        pMainWindow->GetHwnd(),
        nullptr,
        pMainWindow->GetHInstance(),
        pWarningsWindow.get());
    if (hWnd == nullptr)
    {
        ErrorBox(L"CreateWindowExW failed for CWarningsWindow, last error is " + std::to_wstring(GetLastError()));
        return nullptr;
    }

    return pWarningsWindow;
}

void
CWarningsWindow::Open()
{
    const auto& DeviceSymbolInfos = m_pMainWindow->GetDeviceSymbolInfos();

    // Add the warnings to the ListView.
    SendMessageW(m_hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(m_hList);
    int iItem = 0;

    for (const auto& DeviceSymbolInfo : DeviceSymbolInfos)
    {
        std::wstring wstrDevice = StrToWstr(DeviceSymbolInfo.strName);

        for (const CS7PError& Warning : DeviceSymbolInfo.Warnings)
        {
            LVITEMW lvItem = {};
            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = iItem;
            lvItem.pszText = wstrDevice.data();
            ListView_InsertItem(m_hList, &lvItem);

            lvItem.iSubItem++;
            lvItem.pszText = const_cast<LPWSTR>(Warning.Message().c_str());
            ListView_SetItem(m_hList, &lvItem);

            iItem++;
        }
    }

    SendMessageW(m_hList, WM_SETREDRAW, TRUE, 0);

    // Finally, disable the main window and show our window (modal behavior).
    EnableWindow(m_pMainWindow->GetHwnd(), FALSE);
    ShowWindow(m_hWnd, SW_SHOW);
}
