//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include "S7-Project-Explorer.h"

#define IDC_DEVICE_COMBOBOX     500


LRESULT
CVariablesPage::_OnCommand(WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_DEVICE_COMBOBOX:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                OnDeviceComboBoxSelectionChange();
            }

            break;
        }
    }

    return 0;
}

LRESULT
CVariablesPage::_OnCreate()
{
    // Load resources.
    m_wstrHeader = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_VARIABLESPAGE_HEADER);
    m_wstrSubHeader = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_VARIABLESPAGE_SUBHEADER);
    m_wstrText = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_VARIABLESPAGE_TEXT);
    m_wstrSave = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_SAVE);

    // Set up the Device ComboBox.
    m_hDeviceComboBox = CreateWindowExW(0, WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(IDC_DEVICE_COMBOBOX), nullptr, nullptr);
    SendMessageW(m_hDeviceComboBox, WM_SETFONT, reinterpret_cast<WPARAM>(m_pMainWindow->GetGuiFont()), MAKELPARAM(TRUE, 0));

    // Set up the ListView.
    m_hList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, 0, 0, 0, 0, m_hWnd, nullptr, nullptr, nullptr);
    ListView_SetExtendedListViewStyle(m_hList, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    LVCOLUMNW lvColumn = {};
    lvColumn.mask = LVCF_TEXT;

    std::wstring wstrColumn = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_NAME);
    lvColumn.pszText = wstrColumn.data();
    ListView_InsertColumn(m_hList, 0, &lvColumn);

    wstrColumn = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_CODE);
    lvColumn.pszText = wstrColumn.data();
    ListView_InsertColumn(m_hList, 1, &lvColumn);

    wstrColumn = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_DATATYPE);
    lvColumn.pszText = wstrColumn.data();
    ListView_InsertColumn(m_hList, 2, &lvColumn);

    wstrColumn = LoadStringAsWstr(m_pMainWindow->GetHInstance(), IDS_COMMENT);
    lvColumn.pszText = wstrColumn.data();
    ListView_InsertColumn(m_hList, 3, &lvColumn);

    return 0;
}

LRESULT
CVariablesPage::_OnEraseBkgnd()
{
    // We always repaint the entire window in _OnPaint.
    // Therefore, don't forward WM_ERASEBKGND messages to DefWindowProcW to prevent flickering.
    return TRUE;
}

LRESULT
CVariablesPage::_OnPaint()
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

    // Draw the info text.
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
CVariablesPage::_OnSize()
{
    // Get the window size.
    RECT rcWindow;
    GetClientRect(m_hWnd, &rcWindow);

    // The text is drawn on most of the window, so invalidate that.
    InvalidateRect(m_hWnd, &rcWindow, FALSE);

    // Move the Device ComboBox.
    HDWP hDwp = BeginDeferWindowPos(2);
    if (!hDwp)
    {
        return 0;
    }

    int iComboX = 0;
    int iComboY = m_pMainWindow->ScaleFont(30);
    int iComboHeight = m_pMainWindow->ScaleFont(10);
    int iComboWidth = rcWindow.right;
    hDwp = DeferWindowPos(hDwp, m_hDeviceComboBox, nullptr, iComboX, iComboY, iComboWidth, iComboHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    // Move the ListView.
    int iListX = 0;
    int iListY = m_pMainWindow->ScaleFont(50);
    int iListHeight = rcWindow.bottom - iListY;
    int iListWidth = rcWindow.right;
    hDwp = DeferWindowPos(hDwp, m_hList, nullptr, iListX, iListY, iListWidth, iListHeight, 0);
    if (!hDwp)
    {
        return 0;
    }

    EndDeferWindowPos(hDwp);

    // Adjust the list column widths.
    LONG lLargeColumnWidth = rcWindow.right / 3;
    LONG lSmallColumnWidth = lLargeColumnWidth / 2;
    LONG lMediumColumnWidth = lSmallColumnWidth + lSmallColumnWidth / 2;
    ListView_SetColumnWidth(m_hList, 0, lMediumColumnWidth);
    ListView_SetColumnWidth(m_hList, 1, lSmallColumnWidth);
    ListView_SetColumnWidth(m_hList, 2, lSmallColumnWidth);
    ListView_SetColumnWidth(m_hList, 3, lLargeColumnWidth);

    return 0;
}

LRESULT CALLBACK
CVariablesPage::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CVariablesPage* pPage = InstanceFromWndProc<CVariablesPage, CPage, &CVariablesPage::CPage::m_hWnd>(hWnd, uMsg, lParam);

    if (pPage)
    {
        switch (uMsg)
        {
            case WM_COMMAND: return pPage->_OnCommand(wParam);
            case WM_CREATE: return pPage->_OnCreate();
            case WM_ERASEBKGND: return pPage->_OnEraseBkgnd();
            case WM_PAINT: return pPage->_OnPaint();
            case WM_SIZE: return pPage->_OnSize();
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void
CVariablesPage::OnDeviceComboBoxSelectionChange()
{
    // Get the symbol information for the selected device.
    int iSelectedDeviceIndex = ComboBox_GetCurSel(m_hDeviceComboBox);
    const S7DeviceSymbolInfo& DeviceSymbolInfo = m_pMainWindow->GetDeviceSymbolInfos()[iSelectedDeviceIndex];

    SendMessageW(m_hList, WM_SETREDRAW, FALSE, 0);

    // Remove all existing items and groups that may exist from a previous project (the user can click "Back" and select a new project).
    // Windows XP seems to have a bug here in that it forgets about the group view when all groups have been removed. Therefore, we have to reenable group view as well.
    ListView_DeleteAllItems(m_hList);
    ListView_RemoveAllGroups(m_hList);
    ListView_EnableGroupView(m_hList, TRUE);

    // Add the symbols to the Variables ListView.
    int iGroupId = 0;
    int iItem = 0;

    for (const S7Block& Block : DeviceSymbolInfo.Blocks)
    {
        std::wstring wstrHeader = StrToWstr(Block.strName);

        LVGROUP lvGroup = {};
        lvGroup.cbSize = sizeof(LVGROUP);
        lvGroup.mask = LVGF_HEADER | LVGF_GROUPID;
        lvGroup.pszHeader = wstrHeader.data();
        lvGroup.iGroupId = iGroupId;
        ListView_InsertGroup(m_hList, iGroupId, &lvGroup);

        for (const auto& Symbol : Block.Symbols)
        {
            std::wstring wstrName = StrToWstr(Symbol.strName);
            std::wstring wstrCode = StrToWstr(Symbol.strCode);
            std::wstring wstrDatatype = StrToWstr(Symbol.strDatatype);
            std::wstring wstrComment = StrToWstr(Symbol.strComment);

            LVITEMW lvItem = {};
            lvItem.mask = LVIF_TEXT | LVIF_GROUPID;
            lvItem.iItem = iItem;
            lvItem.iGroupId = iGroupId;
            lvItem.pszText = wstrName.data();
            ListView_InsertItem(m_hList, &lvItem);

            lvItem.mask = LVIF_TEXT;

            lvItem.iSubItem++;
            lvItem.pszText = wstrCode.data();
            ListView_SetItem(m_hList, &lvItem);

            lvItem.iSubItem++;
            lvItem.pszText = wstrDatatype.data();
            ListView_SetItem(m_hList, &lvItem);

            lvItem.iSubItem++;
            lvItem.pszText = wstrComment.data();
            ListView_SetItem(m_hList, &lvItem);

            iItem++;
        }

        iGroupId++;
    }

    SendMessageW(m_hList, WM_SETREDRAW, TRUE, 0);
}

void
CVariablesPage::SwitchTo()
{
    m_pMainWindow->SetHeader(&m_wstrHeader, &m_wstrSubHeader);
    m_pMainWindow->EnableBackButton(TRUE);
    m_pMainWindow->SetNextButtonText(m_wstrSave);
    ShowWindow(m_hWnd, SW_SHOW);
}

void
CVariablesPage::UpdateDPI()
{
    // Update the control fonts.
    SendMessageW(m_hDeviceComboBox, WM_SETFONT, reinterpret_cast<WPARAM>(m_pMainWindow->GetGuiFont()), MAKELPARAM(TRUE, 0));
}
