//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

class CFilePage final : public CPage
{
public:
    static std::unique_ptr<CFilePage> Create(CMainWindow* pMainWindow) { return CPage::Create<CFilePage>(pMainWindow); }
    const std::wstring& GetSelectedFilePath() const { return m_wstrSelectedFilePath; }
    void SwitchTo();
    void UpdateDPI();

private:
    friend class CPage;
    static constexpr WCHAR _wszWndClass[] = L"FilePageWndClass";

    HWND m_hFile;
    HWND m_hFileName;
    HWND m_hFileBrowse;
    Gdiplus::Bitmap* m_pCurrentFileBitmap;
    std::unique_ptr<Gdiplus::Bitmap> m_pBlankFileBitmap;
    std::unique_ptr<Gdiplus::Bitmap> m_pS7PFileBitmap;
    std::wstring m_wstrBrowseFilter;
    std::wstring m_wstrBrowseTitle;
    std::wstring m_wstrHeader;
    std::wstring m_wstrSubHeader;
    std::wstring m_wstrText;
    std::wstring m_wstrNext;
    std::wstring m_wstrSelectedFilePath;

    CFilePage(CMainWindow* pMainWindow) : CPage(pMainWindow) {}
    void _OnBrowseButton();
    LRESULT _OnCommand(WPARAM wParam);
    LRESULT _OnCreate();
    LRESULT _OnDrawItem(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnEraseBkgnd();
    LRESULT _OnPaint();
    LRESULT _OnSetCursor(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT _OnSize();
    static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
