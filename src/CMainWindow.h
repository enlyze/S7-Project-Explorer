//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

class CMainWindow
{
public:
    const std::vector<S7DeviceSymbolInfo>& GetDeviceSymbolInfos() const { return m_DeviceSymbolInfos; }
    HFONT GetGuiFont() const { return m_hGuiFont.get(); }
    HINSTANCE GetHInstance() const { return m_hInstance; }
    HWND GetHwnd() const { return m_hWnd; }

    static std::unique_ptr<CMainWindow> Create(HINSTANCE hInstance, int nShowCmd);
    void EnableBackButton(BOOL bEnable);
    void EnableNextButton(BOOL bEnable);
    void EnableCancelButton(BOOL bEnable);
    int ScaleControl(int iControlSize);
    int ScaleFont(int iFontSize);
    void SetHeader(std::wstring* pwstrHeader, std::wstring* pwstrSubHeader);
    void SetNextButtonText(const std::wstring& wstrText);
    void ShowWarningsButton(BOOL bShow);
    int WorkLoop();

private:
    static constexpr WCHAR _wszWndClass[] = L"MainWndClass";

    sr::unique_resource<HFONT, decltype(DeleteObject)*> m_hBoldGuiFont;
    sr::unique_resource<HFONT, decltype(DeleteObject)*> m_hGuiFont;
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    HWND m_hLine;
    HWND m_hWarnings;
    HWND m_hBack;
    HWND m_hNext;
    HWND m_hCancel;
    int m_nShowCmd;
    LOGFONTW m_lfBoldGuiFont;
    LOGFONTW m_lfGuiFont;
    CPage* m_pCurrentPage;
    std::vector<S7DeviceSymbolInfo> m_DeviceSymbolInfos;
    std::unique_ptr<CFilePage> m_pFilePage;
    std::unique_ptr<CVariablesPage> m_pVariablesPage;
    std::unique_ptr<CFinishPage> m_pFinishPage;
    std::unique_ptr<CWarningsWindow> m_pWarningsWindow;
    std::unique_ptr<Gdiplus::Bitmap> m_pLogoBitmap;
    std::wstring m_wstrSelectedFilePath;
    std::wstring m_wstrSaveFilter;
    std::wstring m_wstrSaveTitle;
    std::wstring* m_pwstrHeader;
    std::wstring* m_pwstrSubHeader;
    WORD m_wCurrentDPI;

    CMainWindow(HINSTANCE hInstance, int nShowCmd);
    static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _OnBackButton();
    void _OnCancelButton();
    LRESULT _OnCommand(WPARAM wParam);
    LRESULT _OnCreate();
    LRESULT _OnDestroy();
    LRESULT _OnDpiChanged(WPARAM wParam, LPARAM lParam);
    LRESULT _OnEraseBkgnd();
    LRESULT _OnGetMinMaxInfo(LPARAM lParam);
    void _OnNextButton();
    void _OnFilePageNextButton();
    void _OnVariablesPageNextButton();
    void _OnFinishPageNextButton();
    void _OnWarningsButton();
    LRESULT _OnPaint();
    LRESULT _OnSize();
    void _RedrawHeader();
    void _SwitchPage(CPage* pNewPage);
};
