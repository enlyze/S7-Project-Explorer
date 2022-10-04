//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "targetver.h"
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>

#pragma warning(push)
#pragma warning(disable:4458)
#include <GdiPlus.h>
#pragma warning(pop)

#if !defined(WM_DPICHANGED)
#define WM_DPICHANGED 0x02E0
#endif

#include "unique_resource.h"
#include "win32_wrappers.h"

#include <EnlyzeWinStringLib.h>
#include <s7p_parser.h>

#include "resource.h"
#include "csv_exporter.h"
#include "utils.h"
#include "version.h"

// Forward declarations
class CFilePage;
class CFinishPage;
class CMainWindow;
class CPage;
class CVariablesPage;
class CWarningsWindow;

#include "CPage.h"
#include "CFilePage.h"
#include "CFinishPage.h"
#include "CMainWindow.h"
#include "CVariablesPage.h"
#include "CWarningsWindow.h"

// S7-Project-Explorer.cpp
extern const int iFontReferenceDPI;
extern const int iWindowsReferenceDPI;
extern const int iUnifiedControlPadding;
extern const int iUnifiedButtonHeight;
extern const int iUnifiedButtonWidth;
extern const WCHAR wszAppName[];
