//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include "S7-Project-Explorer.h"

static const int iHeaderHeight = 70;
static const int iMinWindowHeight = 500;
static const int iMinWindowWidth = 700;

#define IDC_BACK        500
#define IDC_NEXT        501
#define IDC_CANCEL      502
#define IDC_WARNINGS    503


CMainWindow::CMainWindow(HINSTANCE hInstance, int nShowCmd)
    : m_hInstance(hInstance), m_nShowCmd(nShowCmd)
{
}

LRESULT CALLBACK
CMainWindow::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMainWindow* pMainWindow = InstanceFromWndProc<CMainWindow, CMainWindow, &CMainWindow::m_hWnd>(hWnd, uMsg, lParam);

    // The first WM_GETMINMAXINFO comes before WM_NCCREATE, before we got our CMainWindow pointer.
    if (pMainWindow)
    {
        switch (uMsg)
        {
            case WM_COMMAND: return pMainWindow->_OnCommand(wParam);
            case WM_CREATE: return pMainWindow->_OnCreate();
            case WM_DESTROY: return pMainWindow->_OnDestroy();
            case WM_DPICHANGED: return pMainWindow->_OnDpiChanged(wParam, lParam);
            case WM_ERASEBKGND: return pMainWindow->_OnEraseBkgnd();
            case WM_GETMINMAXINFO: return pMainWindow->_OnGetMinMaxInfo(lParam);
            case WM_PAINT: return pMainWindow->_OnPaint();
            case WM_SIZE: return pMainWindow->_OnSize();
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void
CMainWindow::_OnBackButton()
{
    if (m_pCurrentPage == m_pVariablesPage.get())
    {
        // Switch back to the first page.
        _SwitchPage(m_pFilePage.get());
    }

    // The Back button is disabled for all other pages, so we don't have to handle these cases.
}

void
CMainWindow::_OnCancelButton()
{
    DestroyWindow(m_hWnd);
}

void
CMainWindow::_OnNextButton()
{
    if (m_pCurrentPage == m_pFilePage.get())
    {
        _OnFilePageNextButton();
    }
    else if (m_pCurrentPage == m_pVariablesPage.get())
    {
        _OnVariablesPageNextButton();
    }
    else if (m_pCurrentPage == m_pFinishPage.get())
    {
        _OnFinishPageNextButton();
    }
}

void
CMainWindow::_OnFilePageNextButton()
{
    // Parse the Step7 project.
    auto ParseResult = ParseS7P(m_pFilePage->GetSelectedFilePath());
    if (const auto pError = std::get_if<CS7PError>(&ParseResult))
    {
        ErrorBox(LoadStringAsWstr(m_hInstance, IDS_PARSE_ERROR) + pError->Message());
        return;
    }

    m_DeviceSymbolInfos = std::get<std::vector<S7DeviceSymbolInfo>>(std::move(ParseResult));
    if (m_DeviceSymbolInfos.empty())
    {
        ErrorBox(LoadStringAsWstr(m_hInstance, IDS_NO_VARIABLES));
        return;
    }

    // Add the devices to the Device ComboBox and count warnings.
    HWND hDeviceComboBox = m_pVariablesPage->GetDeviceComboBox();
    ComboBox_ResetContent(hDeviceComboBox);
    size_t cWarnings = 0;

    for (const auto& DeviceSymbolInfo : m_DeviceSymbolInfos)
    {
        ComboBox_AddString(hDeviceComboBox, StrToWstr(DeviceSymbolInfo.strName).c_str());
        cWarnings += DeviceSymbolInfo.Warnings.size();
    }

    // Select the first device.
    ComboBox_SetCurSel(hDeviceComboBox, 0);
    m_pVariablesPage->OnDeviceComboBoxSelectionChange();

    // Show the "Warnings" button if we have warnings.
    if (cWarnings > 0)
    {
        std::wstring wstrWarnings = std::to_wstring(cWarnings) + L" " + LoadStringAsWstr(m_hInstance, IDS_WARNINGS);
        SetWindowTextW(m_hWarnings, wstrWarnings.c_str());
        SetWindowTextW(m_pWarningsWindow->GetHwnd(), wstrWarnings.c_str());
        ShowWarningsButton(TRUE);
    }

    // Switch to the Variables page.
    _SwitchPage(m_pVariablesPage.get());
}

void
CMainWindow::_OnVariablesPageNextButton()
{
    std::wstring wstrFileToSave = std::wstring(MAX_PATH, L'\0');

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = m_wstrSaveFilter.c_str();
    ofn.lpstrFile = wstrFileToSave.data();
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = m_wstrSaveTitle.c_str();
    ofn.lpstrDefExt = L".csv";

    if (GetSaveFileNameW(&ofn))
    {
        wstrFileToSave.resize(wstrFileToSave.find(L'\0'));

        // Export the list into the chosen file.
        auto ExportResult = ExportCSV(wstrFileToSave, m_DeviceSymbolInfos);
        if (const auto pError = std::get_if<CS7PError>(&ExportResult))
        {
            ErrorBox(LoadStringAsWstr(m_hInstance, IDS_SAVE_ERROR) + pError->Message());
            return;
        }

        // Switch to the Finish page.
        _SwitchPage(m_pFinishPage.get());
    }
}

void
CMainWindow::_OnFinishPageNextButton()
{
    DestroyWindow(m_hWnd);
}

void
CMainWindow::_OnWarningsButton()
{
    m_pWarningsWindow->Open();
}

LRESULT
CMainWindow::_OnCommand(WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_BACK: _OnBackButton(); break;
        case IDC_NEXT: _OnNextButton(); break;
        case IDC_CANCEL: _OnCancelButton(); break;
        case IDC_WARNINGS: _OnWarningsButton(); break;
    }

    return 0;
}

LRESULT
CMainWindow::_OnCreate()
{
    // Get the DPI setting for the monitor where the main window is shown.
    m_wCurrentDPI = GetWindowDPI(m_hWnd);

    // Load resources.
    m_pLogoBitmap = LoadPNGAsGdiplusBitmap(m_hInstance, IDP_LOGO);
    m_wstrSaveFilter = LoadStringAsWstr(m_hInstance, IDS_SAVE_FILTER);
    m_wstrSaveTitle = LoadStringAsWstr(m_hInstance, IDS_SAVE_TITLE);

    // Load the save filter and replace the '|' characters in the resource string by NUL-characters to create the required double-NUL-terminated string.
    // This is needed, because you cannot reliably store double-NUL-terminated strings in resources.
    m_wstrSaveFilter = LoadStringAsWstr(m_hInstance, IDS_SAVE_FILTER);
    std::replace(m_wstrSaveFilter.begin(), m_wstrSaveFilter.end(), L'|', L'\0');

    // Create the main GUI font.
    m_lfGuiFont = {};
    wcscpy_s(m_lfGuiFont.lfFaceName, L"MS Shell Dlg 2");
    m_lfGuiFont.lfHeight = -ScaleFont(10);
    m_hGuiFont = make_unique_font(CreateFontIndirectW(&m_lfGuiFont));

    // Create the bold GUI font.
    m_lfBoldGuiFont = m_lfGuiFont;
    m_lfBoldGuiFont.lfWeight = FW_BOLD;
    m_hBoldGuiFont = make_unique_font(CreateFontIndirectW(&m_lfBoldGuiFont));

    // Create the line above the buttons.
    m_hLine = CreateWindowExW(0, WC_STATICW, L"", WS_CHILD | WS_VISIBLE | SS_SUNKEN, 0, 0, 0, 0, m_hWnd, nullptr, nullptr, nullptr);

    // Create the bottom buttons.
    m_hWarnings = CreateWindowExW(0, WC_BUTTONW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_WARNINGS), nullptr, nullptr);
    SendMessageW(m_hWarnings, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));

    std::wstring wstrBack = LoadStringAsWstr(m_hInstance, IDS_BACK);
    m_hBack = CreateWindowExW(0, WC_BUTTONW, wstrBack.c_str(), WS_CHILD | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_BACK), nullptr, nullptr);
    SendMessageW(m_hBack, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));

    m_hNext = CreateWindowExW(0, WC_BUTTONW, L"", WS_CHILD | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_NEXT), nullptr, nullptr);
    SendMessageW(m_hNext, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));

    std::wstring wstrCancel = LoadStringAsWstr(m_hInstance, IDS_CANCEL);
    m_hCancel = CreateWindowExW(0, WC_BUTTONW, wstrCancel.c_str(), WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_CANCEL), nullptr, nullptr);
    SendMessageW(m_hCancel, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));

    // Create all pages and windows.
    m_pFilePage = CFilePage::Create(this);
    m_pVariablesPage = CVariablesPage::Create(this);
    m_pFinishPage = CFinishPage::Create(this);
    m_pWarningsWindow = CWarningsWindow::Create(this);
    _SwitchPage(m_pFilePage.get());

    // Set the main window size.
    int iHeight = ScaleControl(iMinWindowHeight);
    int iWidth = ScaleControl(iMinWindowWidth);
    SetWindowPos(m_hWnd, nullptr, 0, 0, iWidth, iHeight, SWP_NOMOVE);

    // Finally, show the window.
    ShowWindow(m_hWnd, m_nShowCmd);

    return 0;
}

LRESULT
CMainWindow::_OnDestroy()
{
    PostQuitMessage(0);
    return 0;
}

LRESULT
CMainWindow::_OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
    m_wCurrentDPI = LOWORD(wParam);

    // Redraw the entire window on every DPI change.
    RECT rcWindow;
    GetClientRect(m_hWnd, &rcWindow);
    InvalidateRect(m_hWnd, &rcWindow, FALSE);

    // Recalculate the main GUI font.
    m_lfGuiFont.lfHeight = -ScaleFont(10);
    m_hGuiFont = make_unique_font(CreateFontIndirectW(&m_lfGuiFont));

    // Recalculate the bold GUI font.
    m_lfBoldGuiFont.lfHeight = m_lfGuiFont.lfHeight;
    m_hBoldGuiFont = make_unique_font(CreateFontIndirectW(&m_lfBoldGuiFont));

    // Update the control fonts.
    SendMessageW(m_hWarnings, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));
    SendMessageW(m_hBack, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));
    SendMessageW(m_hNext, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));
    SendMessageW(m_hCancel, WM_SETFONT, reinterpret_cast<WPARAM>(m_hGuiFont.get()), MAKELPARAM(TRUE, 0));

    // Update the DPI for our custom child windows.
    m_pFilePage->UpdateDPI();
    m_pVariablesPage->UpdateDPI();

    // Use the suggested new window size.
    RECT* const prcNewWindow = reinterpret_cast<RECT*>(lParam);
    SetWindowPos(m_hWnd, nullptr, prcNewWindow->left, prcNewWindow->top, prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);

    return 0;
}

LRESULT
CMainWindow::_OnEraseBkgnd()
{
    // We always repaint the entire window in _OnPaint.
    // Therefore, don't forward WM_ERASEBKGND messages to DefWindowProcW to prevent flickering.
    return TRUE;
}

LRESULT
CMainWindow::_OnGetMinMaxInfo(LPARAM lParam)
{
    PMINMAXINFO pMinMaxInfo = reinterpret_cast<PMINMAXINFO>(lParam);

    pMinMaxInfo->ptMinTrackSize.x = ScaleControl(iMinWindowWidth);
    pMinMaxInfo->ptMinTrackSize.y = ScaleControl(iMinWindowHeight);

    return 0;
}

LRESULT
CMainWindow::_OnPaint()
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

    // Draw a white rectangle completely filling the header of the window.
    RECT rcHeader = rcWindow;
    rcHeader.bottom = ScaleControl(iHeaderHeight);
    FillRect(hMemDC, &rcHeader, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

    // Draw the header text.
    RECT rcHeaderText = rcHeader;
    rcHeaderText.left = ScaleControl(15);
    rcHeaderText.top = ScaleControl(15);
    SelectObject(hMemDC, m_hBoldGuiFont.get());
    DrawTextW(hMemDC, m_pwstrHeader->c_str(), m_pwstrHeader->size(), &rcHeaderText, 0);

    // Draw the subheader text.
    RECT rcSubHeaderText = rcHeader;
    rcSubHeaderText.left = ScaleControl(20);
    rcSubHeaderText.top = ScaleControl(32);
    SelectObject(hMemDC, m_hGuiFont.get());
    DrawTextW(hMemDC, m_pwstrSubHeader->c_str(), m_pwstrSubHeader->size(), &rcSubHeaderText, 0);

    // Draw the ENLYZE logo into the upper right corner.
    const int iLogoPadding = ScaleControl(5);
    int iDestBitmapHeight = rcHeader.bottom - 2 * iLogoPadding;
    int iDestBitmapWidth = m_pLogoBitmap->GetWidth() * iDestBitmapHeight / m_pLogoBitmap->GetHeight();
    int iDestBitmapX = rcWindow.right - iLogoPadding - iDestBitmapWidth;
    int iDestBitmapY = iLogoPadding;

    Gdiplus::Graphics g(hMemDC);
    g.DrawImage(m_pLogoBitmap.get(), iDestBitmapX, iDestBitmapY, iDestBitmapWidth, iDestBitmapHeight);

    // Fill the rest of the window with the window background color.
    RECT rcBackground = rcWindow;
    rcBackground.top = rcHeader.bottom;
    FillRect(hMemDC, &rcBackground, GetSysColorBrush(COLOR_BTNFACE));

    // End painting by copying the in-memory DC.
    BitBlt(hDC, 0, 0, rcWindow.right, rcWindow.bottom, hMemDC, 0, 0, SRCCOPY);
    DeleteObject(hMemBitmap);
    DeleteDC(hMemDC);
    EndPaint(m_hWnd, &ps);

    return 0;
}

LRESULT
CMainWindow::_OnSize()
{
    // Get the window size.
    RECT rcWindow;
    GetClientRect(m_hWnd, &rcWindow);

    // Redraw the header on every size change.
    RECT rcHeader = rcWindow;
    rcHeader.bottom = ScaleControl(iHeaderHeight);
    InvalidateRect(m_hWnd, &rcHeader, FALSE);

    // Move the buttons.
    HDWP hDwp = BeginDeferWindowPos(8);
    if (!hDwp)
    {
        return 0;
    }

    const int iControlPadding = ScaleControl(iUnifiedControlPadding);
    const int iButtonHeight = ScaleControl(iUnifiedButtonHeight);
    const int iButtonWidth = ScaleControl(iUnifiedButtonWidth);
    int iButtonX = iControlPadding;
    int iButtonY = rcWindow.bottom - iControlPadding - iButtonHeight;
    hDwp = DeferWindowPos(hDwp, m_hWarnings, nullptr, iButtonX, iButtonY, iButtonWidth, iButtonHeight, 0);

    iButtonX = rcWindow.right - iControlPadding - iButtonWidth;
    hDwp = DeferWindowPos(hDwp, m_hCancel, nullptr, iButtonX, iButtonY, iButtonWidth, iButtonHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    iButtonX = iButtonX - iControlPadding - iButtonWidth;
    hDwp = DeferWindowPos(hDwp, m_hNext, nullptr, iButtonX, iButtonY, iButtonWidth, iButtonHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    iButtonX = iButtonX - iButtonWidth;
    hDwp = DeferWindowPos(hDwp, m_hBack, nullptr, iButtonX, iButtonY, iButtonWidth, iButtonHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    // Move the line above the buttons.
    int iLineHeight = 2;
    int iLineWidth = rcWindow.right;
    int iLineX = 0;
    int iLineY = iButtonY - iControlPadding;
    hDwp = DeferWindowPos(hDwp, m_hLine, nullptr, iLineX, iLineY, iLineWidth, iLineHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    // Move all page windows.
    int iPageX = iControlPadding;
    int iPageY = rcHeader.bottom + iControlPadding;
    int iPageHeight = iLineY - iPageY - iControlPadding;
    int iPageWidth = rcWindow.right - iPageX - iControlPadding;
    hDwp = DeferWindowPos(hDwp, m_pFilePage->GetHwnd(), nullptr, iPageX, iPageY, iPageWidth, iPageHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    hDwp = DeferWindowPos(hDwp, m_pVariablesPage->GetHwnd(), nullptr, iPageX, iPageY, iPageWidth, iPageHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    hDwp = DeferWindowPos(hDwp, m_pFinishPage->GetHwnd(), nullptr, iPageX, iPageY, iPageWidth, iPageHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    EndDeferWindowPos(hDwp);

    return 0;
}

void
CMainWindow::_SwitchPage(CPage* pNewPage)
{
    m_pCurrentPage = pNewPage;
    ShowWindow(m_pFilePage->GetHwnd(), SW_HIDE);
    ShowWindow(m_pVariablesPage->GetHwnd(), SW_HIDE);
    ShowWindow(m_pFinishPage->GetHwnd(), SW_HIDE);
    pNewPage->SwitchTo();
}

std::unique_ptr<CMainWindow>
CMainWindow::Create(HINSTANCE hInstance, int nShowCmd)
{
    // Register the main window class.
    WNDCLASSW wc = {};
    wc.lpfnWndProc = CMainWindow::_WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON));
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = CMainWindow::_wszWndClass;

    if (RegisterClassW(&wc) == 0)
    {
        ErrorBox(L"RegisterClassW failed, last error is " + std::to_wstring(GetLastError()));
        return nullptr;
    }

    // Create the main window.
    auto pMainWindow = std::unique_ptr<CMainWindow>(new CMainWindow(hInstance, nShowCmd));
    HWND hWnd = CreateWindowExW(
        0,
        CMainWindow::_wszWndClass,
        wszAppName,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInstance,
        pMainWindow.get());
    if (hWnd == nullptr)
    {
        ErrorBox(L"CreateWindowExW failed for CMainWindow, last error is " + std::to_wstring(GetLastError()));
        return nullptr;
    }

    return pMainWindow;
}

void
CMainWindow::EnableBackButton(BOOL bEnable)
{
    EnableWindow(m_hBack, bEnable);
}

void
CMainWindow::EnableNextButton(BOOL bEnable)
{
    EnableWindow(m_hNext, bEnable);
}

void
CMainWindow::EnableCancelButton(BOOL bEnable)
{
    EnableWindow(m_hCancel, bEnable);
}

void
CMainWindow::_RedrawHeader()
{
    RECT rcHeader;
    GetClientRect(m_hWnd, &rcHeader);
    rcHeader.bottom = ScaleControl(iHeaderHeight);
    InvalidateRect(m_hWnd, &rcHeader, FALSE);
}

int
CMainWindow::ScaleControl(int iControlSize)
{
    return MulDiv(iControlSize, m_wCurrentDPI, iWindowsReferenceDPI);
}

int
CMainWindow::ScaleFont(int iFontSize)
{
    return MulDiv(iFontSize, m_wCurrentDPI, iFontReferenceDPI);
}

void
CMainWindow::SetHeader(std::wstring* pwstrHeader, std::wstring* pwstrSubHeader)
{
    m_pwstrHeader = pwstrHeader;
    m_pwstrSubHeader = pwstrSubHeader;
    _RedrawHeader();
}

void
CMainWindow::SetNextButtonText(const std::wstring& wstrText)
{
    SetWindowTextW(m_hNext, wstrText.c_str());
}

void
CMainWindow::ShowWarningsButton(BOOL bShow)
{
    ShowWindow(m_hWarnings, bShow ? SW_SHOW : SW_HIDE);
}

int
CMainWindow::WorkLoop()
{
    // Process window messages.
    MSG msg;
    for (;;)
    {
        BOOL bRet = GetMessageW(&msg, 0, 0, 0);
        if (bRet > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        else if (bRet == 0)
        {
            // WM_QUIT message terminated the message loop.
            return msg.wParam;
        }
        else
        {
            ErrorBox(L"GetMessageW failed, last error is " + std::to_wstring(GetLastError()));
            return 1;
        }
    }
}
