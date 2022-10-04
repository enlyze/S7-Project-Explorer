//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

class CVariablesPage final : public CPage
{
public:
    static std::unique_ptr<CVariablesPage> Create(CMainWindow* pMainWindow) { return CPage::Create<CVariablesPage>(pMainWindow); }

    HWND GetDeviceComboBox() const { return m_hDeviceComboBox; }
    HWND GetList() const { return m_hList; }
    void OnDeviceComboBoxSelectionChange();
    void SwitchTo();
    void UpdateDPI();

private:
    friend class CPage;
    static constexpr WCHAR _wszWndClass[] = L"VariablesPageWndClass";

    HWND m_hDeviceComboBox;
    HWND m_hList;
    std::wstring m_wstrHeader;
    std::wstring m_wstrSubHeader;
    std::wstring m_wstrText;
    std::wstring m_wstrSave;

    CVariablesPage(CMainWindow* pMainWindow) : CPage(pMainWindow) {}
    LRESULT _OnCommand(WPARAM wParam);
    LRESULT _OnCreate();
    LRESULT _OnEraseBkgnd();
    LRESULT _OnPaint();
    LRESULT _OnSize();
    static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
