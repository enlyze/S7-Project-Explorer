//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include "S7-Project-Explorer.h"


static std::string
_SanitizeString(std::string str)
{
    str.erase(std::remove(str.begin(), str.end(), ';'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\"'), str.end());
    return str;
}

std::variant<std::monostate, CS7PError>
ExportCSV(const std::wstring& wstrCSVFilePath, const std::vector<S7DeviceSymbolInfo>& DeviceSymbolInfos)
{
    // Convert the variables data into CSV.
    std::string strCSV = "DEVICE;BLOCK;VARIABLE;CODE;DATATYPE;COMMENT\n";

    for (const S7DeviceSymbolInfo& DeviceSymbolInfo : DeviceSymbolInfos)
    {
        std::string strSanitizedDeviceName = _SanitizeString(DeviceSymbolInfo.strName);

        for (const S7Block& Block : DeviceSymbolInfo.Blocks)
        {
            std::string strSanitizedBlockName = _SanitizeString(Block.strName);

            for (const S7Symbol& Symbol : Block.Symbols)
            {
                strCSV += strSanitizedDeviceName + ";";
                strCSV += strSanitizedBlockName + ";";
                strCSV += _SanitizeString(Symbol.strName) + ";";
                strCSV += Symbol.strCode + ";";
                strCSV += Symbol.strDatatype + ";";
                strCSV += _SanitizeString(Symbol.strComment) + "\n";
            }
        }

        // Use the "DEVICE" and "COMMENT" columns to output per-device warnings, leaving all other columns empty.
        for (const CS7PError& Warning : DeviceSymbolInfo.Warnings)
        {
            std::string strWarning = WstrToStr(Warning.Message());
            strCSV += strSanitizedDeviceName + ";";
            strCSV += ";";
            strCSV += ";";
            strCSV += ";";
            strCSV += ";";
            strCSV += strWarning + "\n";
        }
    }

    // Open the output file for writing.
    auto hFile = make_unique_handle(CreateFileW(wstrCSVFilePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (hFile.get() == INVALID_HANDLE_VALUE)
    {
        return CS7PError(L"CreateFileW failed with error " + std::to_wstring(GetLastError()));
    }

    // Write the BOM to indicate UTF-8 output.
    const unsigned char ByteOrderMark[] = { 0xEF, 0xBB, 0xBF };
    DWORD cbWritten;
    if (!WriteFile(hFile.get(), ByteOrderMark, sizeof(ByteOrderMark), &cbWritten, nullptr))
    {
        return CS7PError(L"WriteFile failed for the ByteOrderMark with error " + std::to_wstring(GetLastError()));
    }

    // Write the CSV output.
    if (!WriteFile(hFile.get(), strCSV.c_str(), strCSV.size(), &cbWritten, nullptr))
    {
        return CS7PError(L"WriteFile failed for the CSV content with error " + std::to_wstring(GetLastError()));
    }

    return std::monostate();
}
