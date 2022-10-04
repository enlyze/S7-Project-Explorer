//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include <EnlyzeWinStringLib.h>
#include <CDbfReader.h>

#include "s7p_symbol_list_parser.h"


static std::variant<std::monostate, CS7PError>
_ParseSingleYDBSymbolList(const std::wstring& wstrSymbolListFilePath, std::vector<S7Symbol>& Symbols, std::map<size_t, std::string>& DbNamesMap)
{
    // Parse the SYMLIST.DBF dBASE file.
    auto ReadDbfResult = CDbfReader::ReadDbf(wstrSymbolListFilePath);
    if (const auto pError = std::get_if<CDbfError>(&ReadDbfResult))
    {
        return CS7PError(pError->Message());
    }

    auto Reader = std::get<std::unique_ptr<CDbfReader>>(std::move(ReadDbfResult));

    // Get some indexes.
    auto GetIndexResult = Reader->GetFieldIndex("_SKZ");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SYMLIST.DBF: " + pError->Message());
    }

    size_t SkzIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("_OPIEC");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SYMLIST.DBF: " + pError->Message());
    }

    size_t OpiecIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("_DATATYP");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SYMLIST.DBF: " + pError->Message());
    }

    size_t DatatypeIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("_COMMENT");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SYMLIST.DBF: " + pError->Message());
    }

    size_t CommentIndex = std::get<size_t>(GetIndexResult);

    // Iterate through all records.
    for (;;)
    {
        // Read a record containing symbol information.
        auto ReadResult = Reader->ReadNextRecord();
        if (const auto pError = std::get_if<CDbfError>(&ReadResult))
        {
            return CS7PError(L"Could not read from SYMLIST.DBF: " + pError->Message());
        }

        if (std::holds_alternative<std::monostate>(ReadResult))
        {
            // We have read all records, so we are done!
            return std::monostate();
        }

        auto Record = std::get<std::vector<std::string>>(std::move(ReadResult));

        // Get and sanitize the symbol code.
        std::string strCode = Record[OpiecIndex];
        strCode.erase(std::remove(strCode.begin(), strCode.end(), ' '), strCode.end());

        // Only add inputs, memory ("Merker"), and output symbols to the Symbols vector.
        if (const char c = *strCode.c_str(); c == 'I' || c == 'M' || c == 'Q')
        {
            S7Symbol& Symbol = Symbols.emplace_back();
            Symbol.strName = Str1252ToStr(Record[SkzIndex]);
            Symbol.strCode = std::move(strCode);
            Symbol.strDatatype = Record[DatatypeIndex];
            Symbol.strComment = Str1252ToStr(Record[CommentIndex]);
        }
        else if (strCode.starts_with("DB"))
        {
            // The YDB symlist also contains names for the DBs we parse later (in _ParseOmbstx).
            // Save these names in the DbNames map.
            auto Option = StrToSizeT(strCode.substr(2));
            if (Option.has_value())
            {
                size_t DbNumber = Option.value();
                std::string strName = Str1252ToStr(Record[SkzIndex]);
                DbNamesMap[DbNumber] = std::move(strName);
            }
        }
    }
}

std::variant<std::monostate, CS7PError>
ParseYDBs(std::vector<S7DeviceSymbolInfo>& DeviceSymbolInfos, const std::vector<S7DeviceIdInfo>& DeviceIdInfos, const std::wstring& wstrS7PFolderPath)
{
    // Parse the SYMLISTS.DBF dBASE file.
    auto ReadDbfResult = CDbfReader::ReadDbf(wstrS7PFolderPath + L"\\YDBs\\SYMLISTS.DBF");
    if (const auto pError = std::get_if<CDbfError>(&ReadDbfResult))
    {
        return CS7PError(pError->Message());
    }

    auto Reader = std::get<std::unique_ptr<CDbfReader>>(std::move(ReadDbfResult));

    // Get the indexes we are interested in.
    auto GetIndexResult = Reader->GetFieldIndex("_ID");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SYMLISTS.DBF: " + pError->Message());
    }

    size_t IdIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("_DBPATH");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SYMLISTS.DBF: " + pError->Message());
    }

    size_t DbPathIndex = std::get<size_t>(GetIndexResult);

    // Iterate through all records.
    for (;;)
    {
        // Get the Symbol Table ID and database path (usually equal but not necessarily, see e.g. the Zes01_05 example project)
        auto ReadResult = Reader->ReadNextRecord();
        if (const auto pError = std::get_if<CDbfError>(&ReadResult))
        {
            return CS7PError(L"Could not read from SYMLISTS.DBF: " + pError->Message());
        }

        if (std::holds_alternative<std::monostate>(ReadResult))
        {
            break;
        }

        auto Record = std::get<std::vector<std::string>>(std::move(ReadResult));

        // Convert it to a size_t.
        auto Option = StrToSizeT(Record[IdIndex]);
        if (!Option.has_value())
        {
            return CS7PError(L"Invalid SYMLISTS.DBF _ID: " + StrToWstr(Record[IdIndex]));
        }

        size_t SymbolListId = Option.value();

        // Construct the full path to the SYMLIST.DBF in the Symbol List's subdirectory.
        std::wstring wstrSymbolListFilePath = wstrS7PFolderPath;
        wstrSymbolListFilePath += L"\\YDBs\\";
        wstrSymbolListFilePath += StrToWstr(Record[DbPathIndex]);
        wstrSymbolListFilePath += L"\\SYMLIST.DBF";

        // Find the Device that corresponds to this Symbol List.
        const auto& DeviceIdInfoIt = std::find_if(DeviceIdInfos.begin(), DeviceIdInfos.end(), [&](const S7DeviceIdInfo& other)
        {
            return other.SymbolListId == SymbolListId;
        });

        if (DeviceIdInfoIt == DeviceIdInfos.end())
        {
            return CS7PError(L"Could not find DeviceIdInfo for Symbol List " + std::to_wstring(SymbolListId));
        }

        // Add it to the final DeviceSymbolInfos vector.
        S7DeviceSymbolInfo& DeviceSymbolInfo = DeviceSymbolInfos.emplace_back();
        DeviceSymbolInfo.strName = DeviceIdInfoIt->strName;

        // Parse this Symbol List.
        S7Block& Block = DeviceSymbolInfo.Blocks.emplace_back();
        Block.strName = "Symbol List";

        auto ParseResult = _ParseSingleYDBSymbolList(wstrSymbolListFilePath, Block.Symbols, DeviceSymbolInfo.DbNamesMap);
        if (const auto pError = std::get_if<CS7PError>(&ParseResult))
        {
            return *pError;
        }
    }

    return std::monostate();
}
