//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

class CFinishPage final : public CPage
{
public:
    static std::unique_ptr<CFinishPage> Create(CMainWindow* pMainWindow) { return CPage::Create<CFinishPage>(pMainWindow); }
    void SwitchTo();
    void UpdateDPI();

private:
    friend class CPage;
    static constexpr WCHAR _wszWndClass[] = L"FinishPageWndClass";

    std::wstring m_wstrHeader;
    std::wstring m_wstrSubHeader;
    std::wstring m_wstrText;
    std::wstring m_wstrFinish;

    CFinishPage(CMainWindow* pMainWindow) : CPage(pMainWindow) {}
    LRESULT _OnCreate();
    LRESULT _OnEraseBkgnd();
    LRESULT _OnPaint();
    static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
