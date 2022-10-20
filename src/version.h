//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#define APP_MAJOR_VERSION       2
#define APP_MINOR_VERSION       2


// The following macro magic turns arbitrary preprocessor constants into strings.
#define STRINGIFY_INTERNAL(x)   #x
#define STRINGIFY(x)            STRINGIFY_INTERNAL(x)
#define WSTRINGIFY_INTERNAL(x)  L##x
#define WSTRINGIFY(x)           WSTRINGIFY_INTERNAL(x)

#define APP_VERSION_STRING      STRINGIFY(APP_MAJOR_VERSION) "." STRINGIFY(APP_MINOR_VERSION)
#define APP_VERSION_WSTRING     WSTRINGIFY(APP_VERSION_STRING)

#define APP_REVISION_STRING     "unknown revision"

#define APP_VERSION_COMBINED    APP_VERSION_STRING " (" APP_REVISION_STRING ")"
