//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <EnlyzeWinStringLib.h>
#include <CDbfReader.h>

#include "CMc5codeParser.h"
#include "s7p_db_parser.h"


static std::variant<std::monostate, CS7PError>
_ParseSingleDB(std::vector<S7Symbol>& Symbols, const size_t DbNumber, const std::string& strMc5code, const std::map<std::string, std::map<size_t, std::string>>& Mc5codeMap)
{
    size_t BitAddressCounter = 0;

    CMc5codeParser Parser(Symbols, BitAddressCounter, DbNumber, strMc5code, Mc5codeMap);
    auto Result = Parser.Parse();
    if (const auto pError = std::get_if<CS7PError>(&Result))
    {
        return *pError;
    }

    return std::monostate();
}

// A DB reference info subblock (subblock type "00066") has a weird format, which I didn't fully understand yet.
// However, when the DB block Mc5code is empty, testing has shown that the DB reference info subblock contains a reference to an FB block just at the beginning.
// This function is only meant to extract that reference.
static bool
_ExtractFBFromDBReferenceMap(const std::map<size_t, std::string>& DbReferenceMc5codeMap, const size_t DbNumber, size_t& FbNumber)
{
    // Check if we have a DB reference info subblock for this DB number.
    const auto it = DbReferenceMc5codeMap.find(DbNumber);
    if (it == DbReferenceMc5codeMap.end())
    {
        return false;
    }

    const std::string& strDbReferenceMc5code = it->second;

    // Check if this DB reference info subblock begins with a reference to an FB block.
    if (!strDbReferenceMc5code.starts_with("FB"))
    {
        return false;
    }

    // Extract the FB number string.
    const char* pszStart = strDbReferenceMc5code.c_str() + 2;
    const char* pszCurrent = pszStart;
    while (*pszCurrent && isdigit(*pszCurrent))
    {
        pszCurrent++;
    }

    size_t Length = pszCurrent - pszStart;
    if (Length == 0)
    {
        return false;
    }

    std::string strNumber = std::string(pszStart, Length);

    // Convert it to a size_t.
    auto Option = StrToSizeT(strNumber);
    if (!Option.has_value())
    {
        return false;
    }

    FbNumber = Option.value();
    return true;
}

static std::variant<std::monostate, CS7PError>
_ParseDBs(S7DeviceSymbolInfo& DeviceSymbolInfo, const std::map<std::string, std::map<size_t, std::string>>& Mc5codeMap)
{
    const std::map<size_t, std::string>& DbMc5codeMap = Mc5codeMap.at("DB");
    const std::map<size_t, std::string>& DbReferenceMc5codeMap = Mc5codeMap.at("DBREF");

    for (const auto& [DbNumber, strMc5code] : DbMc5codeMap)
    {
        std::variant<std::monostate, CS7PError> Result;
        std::vector<S7Symbol> Symbols;
        size_t FbNumber;

        if (!strMc5code.empty())
        {
            // Parse the MC5 Code for this DB.
            Result = _ParseSingleDB(Symbols, DbNumber, strMc5code, Mc5codeMap);
        }
        else if (_ExtractFBFromDBReferenceMap(DbReferenceMc5codeMap, DbNumber, FbNumber))
        {
            // Find the referenced FB block.
            const std::map<size_t, std::string>& FbMc5codeMap = Mc5codeMap.at("FB");
            const auto it = FbMc5codeMap.find(FbNumber);
            if (it == FbMc5codeMap.end())
            {
                // Ignore this DB, but extract all possible information from the remaining ones.
                DeviceSymbolInfo.Warnings.push_back(
                    CS7PError(
                        L"Could not find referenced FB" + std::to_wstring(FbNumber) +
                        L" while parsing DB" + std::to_wstring(DbNumber)
                    )
                );
                continue;
            }

            // Parse this DB using the MC5 Code of the referenced FB block.
            Result = _ParseSingleDB(Symbols, DbNumber, it->second, Mc5codeMap);
        }
        else
        {
            // This DB apparently has no information we can use, so continue with the next one.
            continue;
        }

        if (const auto pError = std::get_if<CS7PError>(&Result))
        {
            // We couldn't completely extract information for this DB - note down a warning.
            // Anyway, we may have successfully extracted the first few symbols, so add what we have.
            // And also try to extract all possible information from the remaining DBs.
            DeviceSymbolInfo.Warnings.push_back(*pError);
        }

        if (Symbols.empty())
        {
            continue;
        }

        // Construct the block name, which is "DB#" and a human-readable name appended (if available from the DbNamesMap).
        S7Block& Block = DeviceSymbolInfo.Blocks.emplace_back();
        Block.strName = "DB" + std::to_string(DbNumber);

        const auto it = DeviceSymbolInfo.DbNamesMap.find(DbNumber);
        if (it != DeviceSymbolInfo.DbNamesMap.end())
        {
            Block.strName += " (" + it->second + ")";
        }

        // Insert the symbols for this block.
        Block.Symbols = std::move(Symbols);
    }

    return std::monostate();
}

static std::variant<std::monostate, CS7PError>
_ParseSingleOmbstxSubblock(S7DeviceSymbolInfo& DeviceSymbolInfo, const std::wstring& wstrSubblockFilePath)
{
    // Parse the SUBBLK.DBF dBASE file.
    auto ReadDbfResult = CDbfReader::ReadDbf(wstrSubblockFilePath);
    if (const auto pError = std::get_if<CDbfError>(&ReadDbfResult))
    {
        return CS7PError(pError->Message());
    }

    auto Reader = std::get<std::unique_ptr<CDbfReader>>(std::move(ReadDbfResult));

    // Get some indexes.
    auto GetIndexResult = Reader->GetFieldIndex("SUBBLKTYP");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SUBBLK.DBF: " + pError->Message());
    }

    size_t SubblktypIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("BLKNUMBER");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SUBBLK.DBF: " + pError->Message());
    }

    size_t BlknumberIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("MC5LEN");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SUBBLK.DBF: " + pError->Message());
    }

    size_t Mc5lenIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("MC5CODE");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"SUBBLK.DBF: " + pError->Message());
    }

    size_t Mc5codeIndex = std::get<size_t>(GetIndexResult);

    // Iterate through all records to collect DB and UDT code information.
    std::map<size_t, std::string> DbMc5codeMap;
    std::map<size_t, std::string> DbReferenceMc5codeMap;
    std::map<size_t, std::string> FbMc5codeMap;
    std::map<size_t, std::string> SfbMc5codeMap;
    std::map<size_t, std::string> UdtMc5codeMap;

    for (;;)
    {
        // Read a record.
        auto ReadResult = Reader->ReadNextRecord();
        if (const auto pError = std::get_if<CDbfError>(&ReadResult))
        {
            return CS7PError(L"Could not read from " + wstrSubblockFilePath + L": " + pError->Message());
        }

        if (std::holds_alternative<std::monostate>(ReadResult))
        {
            // No more records, we are done!
            break;
        }

        auto Record = std::get<std::vector<std::string>>(std::move(ReadResult));

        // Get the BLKNUMBER column and try to convert it to a size_t.
        auto Option = StrToSizeT(Record[BlknumberIndex]);
        if (!Option.has_value())
        {
            // It can't be converted, so this can't be a record we are interested in.
            continue;
        }

        size_t BlockNumber = Option.value();

        // Get the MC5LEN column and try to convert it to a size_t.
        Option = StrToSizeT(Record[Mc5lenIndex]);
        if (!Option.has_value())
        {
            // It can't be converted, so this can't be a record we are interested in.
            continue;
        }

        size_t BlockLength = Option.value();

        // Truncate the MC5 code to the block length.
        std::string strMc5code = std::move(Record[Mc5codeIndex]);
        if (BlockLength < strMc5code.size())
        {
            strMc5code.resize(BlockLength);
        }

        // What type of record is this?
        if (Record[SubblktypIndex] == "00006")
        {
            // This is a DB block (data block).
            // It's basically everything we are interested in, but its Mc5code may reference FB and UDT blocks.
            // And if it's empty, we have to look into the record with the same block number and subblock type index "00066".
            DbMc5codeMap[BlockNumber] = strMc5code;
        }
        else if (Record[SubblktypIndex] == "00066")
        {
            // This is part of a DB block.
            // This subblock type contains information about all blocks referenced by a DB block.
            // We are only interested in it if a DB block is empty and this subblock contains a reference to an FB block instead.
            DbReferenceMc5codeMap[BlockNumber] = strMc5code;
        }
        else if (Record[SubblktypIndex] == "00004")
        {
            // This is an FB block (function block).
            FbMc5codeMap[BlockNumber] = strMc5code;
        }
        else if (Record[SubblktypIndex] == "00009")
        {
            // This is an SFB block (system function block).
            SfbMc5codeMap[BlockNumber] = strMc5code;
        }
        else if (Record[SubblktypIndex] == "00001")
        {
            // This is a UDT block (custom datatype).
            UdtMc5codeMap[BlockNumber] = strMc5code;
        }
    }

    // Put them into one big map to rule them all!
    std::map<std::string, std::map<size_t, std::string>> Mc5codeMap;
    Mc5codeMap["DB"] = std::move(DbMc5codeMap);
    Mc5codeMap["DBREF"] = std::move(DbReferenceMc5codeMap);
    Mc5codeMap["FB"] = std::move(FbMc5codeMap);
    Mc5codeMap["SFB"] = std::move(SfbMc5codeMap);
    Mc5codeMap["UDT"] = std::move(UdtMc5codeMap);

    // Parse all DBs from the MC5 Code.
    auto ParseResult = _ParseDBs(DeviceSymbolInfo, Mc5codeMap);
    if (const auto pError = std::get_if<CS7PError>(&ParseResult))
    {
        return *pError;
    }

    return std::monostate();
}

std::variant<std::monostate, CS7PError>
ParseOmbstx(std::vector<S7DeviceSymbolInfo>& DeviceSymbolInfos, const std::vector<S7DeviceIdInfo>& DeviceIdInfos, const std::wstring& wstrS7PFolderPath)
{
    // Parse the BSTCNTOF.DBF dBASE file.
    auto ReadDbfResult = CDbfReader::ReadDbf(wstrS7PFolderPath + L"\\ombstx\\offline\\BSTCNTOF.DBF");
    if (const auto pError = std::get_if<CDbfError>(&ReadDbfResult))
    {
        return CS7PError(pError->Message());
    }

    auto Reader = std::get<std::unique_ptr<CDbfReader>>(std::move(ReadDbfResult));

    // Get the index of the ID field.
    auto GetIndexResult = Reader->GetFieldIndex("ID");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(L"BSTCNTOF.DBF: " + pError->Message());
    }

    size_t IdIndex = std::get<size_t>(GetIndexResult);

    // Iterate through all records.
    for (;;)
    {
        // Get the Subblock List ID (which is also the path to the Subblock List).
        auto ReadResult = Reader->ReadNextRecord();
        if (const auto pError = std::get_if<CDbfError>(&ReadResult))
        {
            return CS7PError(L"Could not read from BSTCNTOF.DBF: " + pError->Message());
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
            return CS7PError(L"Invalid BSTCNFOF.DBF ID: " + StrToWstr(Record[IdIndex]));
        }

        size_t SubblockListId = Option.value();

        // Construct the full path to the SUBBLK.DBF in the Subblock List subdirectory.
        // (e.g. "...\ombstx\offline\00000005\SUBBLK.DBF")
        std::wostringstream wssSubblockFilePath;
        wssSubblockFilePath << wstrS7PFolderPath << L"\\ombstx\\offline\\";
        wssSubblockFilePath << std::hex << std::setfill(L'0') << std::setw(8) << SubblockListId;
        wssSubblockFilePath << L"\\SUBBLK.DBF";

        // Find the Device that corresponds to this Subblock List.
        const auto& DeviceIdInfoIt = std::find_if(DeviceIdInfos.begin(), DeviceIdInfos.end(), [&](const S7DeviceIdInfo& other)
        {
            return other.SubblockListId == SubblockListId;
        });

        if (DeviceIdInfoIt == DeviceIdInfos.end())
        {
            // BSTCNTOF.DBF references a Subblock List that has no corresponding Device in the other files.
            // I've had this situation quite a few times during testing.
            // However, the SUBBLK.DBF of those Subblock Lists without a Device has always been empty, so no information is lost if we just silently skip them.
            continue;
        }

        // Find the corresponding entry in the DeviceSymbolInfos vector.
        const std::string& strDeviceName = DeviceIdInfoIt->strName;
        auto DeviceSymbolInfoIt = std::find_if(DeviceSymbolInfos.begin(), DeviceSymbolInfos.end(), [&](const S7DeviceSymbolInfo& other)
        {
            return other.strName == strDeviceName;
        });

        if (DeviceSymbolInfoIt == DeviceSymbolInfos.end())
        {
            return CS7PError(
                L"Could not find DeviceSymbolInfo for \"" + StrToWstr(strDeviceName) +
                L"\" and Subblock List " + std::to_wstring(SubblockListId)
            );
        }

        // Parse this subblock.
        auto ParseResult = _ParseSingleOmbstxSubblock(*DeviceSymbolInfoIt, wssSubblockFilePath.str());
        if (const auto pError = std::get_if<CS7PError>(&ParseResult))
        {
            return *pError;
        }
    }

    return std::monostate();
}
