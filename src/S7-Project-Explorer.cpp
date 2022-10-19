//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include "S7-Project-Explorer.h"

const int iFontReferenceDPI = 72;
const int iWindowsReferenceDPI = 96;

const int iUnifiedControlPadding = 10;
const int iUnifiedButtonHeight = 23;
const int iUnifiedButtonWidth = 100;

const WCHAR wszAppName[] = L"ENLYZE S7-Project-Explorer " APP_VERSION_WSTRING;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nShowCmd)
{
    int iReturnValue = 1;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize GDI+.
    Gdiplus::GdiplusStartupInput gpStartupInput;
    ULONG_PTR gpToken;
    Gdiplus::GdiplusStartup(&gpToken, &gpStartupInput, nullptr);

    // Initialize the standard controls we use.
    // Required for at least Windows XP.
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    // Create the main window and let it handle the rest.
    {
        auto pMainWindow = CMainWindow::Create(hInstance, nShowCmd);
        if (pMainWindow)
        {
            iReturnValue = pMainWindow->WorkLoop();
        }
    }

    // Cleanup
    Gdiplus::GdiplusShutdown(gpToken);

    return iReturnValue;
}
