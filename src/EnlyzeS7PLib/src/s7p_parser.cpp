//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include <EnlyzeWinStringLib.h>

#include "s7p_db_parser.h"
#include "s7p_device_id_info_parser.h"
#include "s7p_parser.h"
#include "s7p_symbol_list_parser.h"


static std::variant<std::monostate, CS7PError>
_GetS7PFolderPath(std::wstring& wstrS7PFolderPath, const std::wstring& wstrS7PFilePath)
{
    size_t BackslashPosition = wstrS7PFilePath.find_last_of(L'\\');
    if (BackslashPosition == std::wstring::npos)
    {
        return CS7PError(L"Did not find any backslash in the .s7p file path");
    }

    wstrS7PFolderPath = wstrS7PFilePath.substr(0, BackslashPosition);
    return std::monostate();
}


std::variant<std::vector<S7DeviceSymbolInfo>, CS7PError>
ParseS7P(const std::wstring& wstrS7PFilePath)
{
    // Get the .s7p folder path for subsequent calls.
    std::wstring wstrS7PFolderPath;
    auto Result = _GetS7PFolderPath(wstrS7PFolderPath, wstrS7PFilePath);
    if (const auto pError = std::get_if<CS7PError>(&Result))
    {
        return *pError;
    }

    // Get the names of all PLCs in this project and their corresponding Symbol List IDs and Subblock List IDs.
    std::vector<S7DeviceIdInfo> DeviceIdInfos;
    Result = ParseDeviceIdInfos(DeviceIdInfos, wstrS7PFolderPath);
    if (const auto pError = std::get_if<CS7PError>(&Result))
    {
        return *pError;
    }

    // Parse the Symbol Tables in the YDBs directory.
    std::vector<S7DeviceSymbolInfo> DeviceSymbolInfos;
    Result = ParseYDBs(DeviceSymbolInfos, DeviceIdInfos, wstrS7PFolderPath);
    if (const auto pError = std::get_if<CS7PError>(&Result))
    {
        return *pError;
    }

    // Parse the Subblock Lists in the ombstx directory.
    Result = ParseOmbstx(DeviceSymbolInfos, DeviceIdInfos, wstrS7PFolderPath);
    if (const auto pError = std::get_if<CS7PError>(&Result))
    {
        return *pError;
    }

    return DeviceSymbolInfos;
}
