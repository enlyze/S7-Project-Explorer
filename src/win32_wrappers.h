//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

static inline auto make_unique_font(HFONT hFont)
{
    return sr::make_unique_resource_checked(hFont, nullptr, DeleteObject);
}

static inline auto make_unique_handle(HANDLE h)
{
    return sr::make_unique_resource_checked(h, INVALID_HANDLE_VALUE, CloseHandle);
}
