//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include <array>
#include <fstream>
#include <map>
#include <EnlyzeWinStringLib.h>
#include <CDbfReader.h>

#include "s7p_device_id_info_parser.h"

struct IntermediateInfo
{
    std::string strName;
    std::string strObjId;
    std::string strObjTyp;
};


static bool
_FileExists(const std::wstring& wstrFilePath)
{
    // libc++'s std::filesystem doesn't build for Windows yet, so we have to use this inefficient approach :(
    std::ifstream f(wstrFilePath.c_str());
    return f.good();
}

static std::variant<std::monostate, CS7PError>
_ParseStations(std::vector<IntermediateInfo>& StationInfos, const std::wstring& wstrDbfFilePath)
{
    static const std::map<std::string, std::string> S7ObjTypMap = {
        { "1314969", "S7-300" },
        { "1314970", "S7-400" },
        { "1315650", "S7-400H" },
        { "1315651", "S7-PC" }
    };

    // Read the HOBJECT1.DBF file.
    auto ReadDbfResult = CDbfReader::ReadDbf(wstrDbfFilePath);
    if (const auto pError = std::get_if<CDbfError>(&ReadDbfResult))
    {
        return CS7PError(pError->Message());
    }

    auto Reader = std::get<std::unique_ptr<CDbfReader>>(std::move(ReadDbfResult));

    // Get the indexes we are interested in.
    auto GetIndexResult = Reader->GetFieldIndex("ID");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t IdIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("OBJTYP");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t ObjTypIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("NAME");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t NameIndex = std::get<size_t>(GetIndexResult);

    // Iterate through all records and collect information about potentially interesting ones.
    for (;;)
    {
        auto ReadResult = Reader->ReadNextRecord();
        if (const auto pError = std::get_if<CDbfError>(&ReadResult))
        {
            return CS7PError(pError->Message());
        }

        if (std::holds_alternative<std::monostate>(ReadResult))
        {
            break;
        }

        auto Record = std::get<std::vector<std::string>>(std::move(ReadResult));

        // Check if this station is an S7 PLC.
        const auto it = S7ObjTypMap.find(Record[ObjTypIndex]);
        if (it == S7ObjTypMap.end())
        {
            continue;
        }

        // Yes, then collect information about it.
        IntermediateInfo& Info = StationInfos.emplace_back();
        Info.strName = it->second + ": " + Str1252ToStr(Record[NameIndex]);
        Info.strObjId = Record[IdIndex];
        Info.strObjTyp = Record[ObjTypIndex];
    }

    return std::monostate();
}

static std::variant<std::monostate, CS7PError>
_ParseRelations(std::vector<IntermediateInfo>& RelationInfos, const std::wstring& wstrDbfFilePath, std::vector<IntermediateInfo>& PreviousInfos, const std::string& strRelID)
{
    // Read the HRELATI1.DBF file.
    auto ReadDbfResult = CDbfReader::ReadDbf(wstrDbfFilePath);
    if (const auto pError = std::get_if<CDbfError>(&ReadDbfResult))
    {
        return CS7PError(pError->Message());
    }

    auto Reader = std::get<std::unique_ptr<CDbfReader>>(std::move(ReadDbfResult));

    // Get the indexes we are interested in.
    auto GetIndexResult = Reader->GetFieldIndex("SOBJID");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t SObjIdIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("SOBJTYP");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t SObjTypIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("RELID");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t RelIdIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("TOBJID");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t TObjIdIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("TOBJTYP");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t TObjTypIndex = std::get<size_t>(GetIndexResult);

    // Iterate through all records and previous infos, and collect information about matching ones.
    for (;;)
    {
        auto ReadResult = Reader->ReadNextRecord();
        if (const auto pError = std::get_if<CDbfError>(&ReadResult))
        {
            return CS7PError(pError->Message());
        }

        if (std::holds_alternative<std::monostate>(ReadResult))
        {
            break;
        }

        auto Record = std::get<std::vector<std::string>>(std::move(ReadResult));

        for (const auto& PreviousInfo : PreviousInfos)
        {
            // Does the source part of the record refer to an object we collected from the previous database?
            if (Record[SObjIdIndex] != PreviousInfo.strObjId)
            {
                continue;
            }

            if (Record[SObjTypIndex] != PreviousInfo.strObjTyp)
            {
                continue;
            }

            // Is the relation ID the one we are looking for?
            if (Record[RelIdIndex] != strRelID)
            {
                continue;
            }

            // All fine, then collect information about the target object.
            IntermediateInfo& Info = RelationInfos.emplace_back();
            Info.strName = PreviousInfo.strName;
            Info.strObjId = Record[TObjIdIndex];
            Info.strObjTyp = Record[TObjTypIndex];
        }
    }

    return std::monostate();
}

static std::variant<std::monostate, CS7PError>
_ParseDevices(std::vector<IntermediateInfo>& DeviceInfos, const std::wstring& wstrDbfFilePath, std::vector<IntermediateInfo>& StationRelationInfos)
{
    // Read the HOBJECT1.DBF file.
    auto ReadDbfResult = CDbfReader::ReadDbf(wstrDbfFilePath);
    if (const auto pError = std::get_if<CDbfError>(&ReadDbfResult))
    {
        return CS7PError(pError->Message());
    }

    auto Reader = std::get<std::unique_ptr<CDbfReader>>(std::move(ReadDbfResult));

    // Get the indexes we are interested in.
    auto GetIndexResult = Reader->GetFieldIndex("ID");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t IdIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("OBJTYP");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t ObjTypIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = Reader->GetFieldIndex("NAME");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t NameIndex = std::get<size_t>(GetIndexResult);

    // Iterate through all records and previous infos, and collect information about matching ones.
    for (;;)
    {
        auto ReadResult = Reader->ReadNextRecord();
        if (const auto pError = std::get_if<CDbfError>(&ReadResult))
        {
            return CS7PError(pError->Message());
        }

        if (std::holds_alternative<std::monostate>(ReadResult))
        {
            break;
        }

        auto Record = std::get<std::vector<std::string>>(std::move(ReadResult));

        for (const auto& PreviousInfo : StationRelationInfos)
        {
            // Does the record refer to an object we collected from the previous database?
            if (Record[IdIndex] != PreviousInfo.strObjId)
            {
                continue;
            }

            if (Record[ObjTypIndex] != PreviousInfo.strObjTyp)
            {
                continue;
            }

            // Yes, then extend the name and collect it.
            IntermediateInfo& Info = DeviceInfos.emplace_back();
            Info.strName = PreviousInfo.strName + " -> " + Str1252ToStr(Record[NameIndex]);
            Info.strObjId = PreviousInfo.strObjId;
            Info.strObjTyp = PreviousInfo.strObjTyp;
        }
    }

    return std::monostate();
}

static std::variant<std::monostate, CS7PError>
_ParseResoffAndLinkhrs(std::vector<S7DeviceIdInfo>& DeviceIdInfos, const std::wstring& wstrS7PFolderPath, std::vector<IntermediateInfo>& PreviousInfos)
{
    static const uint32_t SubblockListIdMagic = 0x00116001;
    static const uint32_t SymbolListIdMagic = 0x00113001;

    // Read the S7RESOFF.DBF file.
    auto ReadDbfResult = CDbfReader::ReadDbf(wstrS7PFolderPath + L"\\hrs\\S7RESOFF.DBF");
    if (const auto pError = std::get_if<CDbfError>(&ReadDbfResult))
    {
        return CS7PError(pError->Message());
    }

    auto ResoffReader = std::get<std::unique_ptr<CDbfReader>>(std::move(ReadDbfResult));

    // Get the indexes we are interested in.
    auto GetIndexResult = ResoffReader->GetFieldIndex("ID");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t IdIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = ResoffReader->GetFieldIndex("NAME");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t NameIndex = std::get<size_t>(GetIndexResult);

    GetIndexResult = ResoffReader->GetFieldIndex("RSRVD4_L");
    if (const auto pError = std::get_if<CDbfError>(&GetIndexResult))
    {
        return CS7PError(pError->Message());
    }

    size_t Rsrvd4LIndex = std::get<size_t>(GetIndexResult);

    // Open the linkhrs.lnk file.
    std::wstring wstrLinkhrsFilePath = wstrS7PFolderPath + L"\\hrs\\linkhrs.lnk";
    std::ifstream Linkhrs(wstrLinkhrsFilePath.c_str(), std::ios::binary);
    if (!Linkhrs)
    {
        return CS7PError(L"Could not open linkhrs.lnk");
    }

    // Iterate through all records.
    for (;;)
    {
        auto ReadResult = ResoffReader->ReadNextRecord();
        if (const auto pError = std::get_if<CDbfError>(&ReadResult))
        {
            return CS7PError(pError->Message());
        }

        if (std::holds_alternative<std::monostate>(ReadResult))
        {
            break;
        }

        auto Record = std::get<std::vector<std::string>>(std::move(ReadResult));

        // Try to find a matching device from the previous device infos to build a full name.
        S7DeviceIdInfo& Info = DeviceIdInfos.emplace_back();
        for (const auto& PreviousInfo : PreviousInfos)
        {
            if (Record[IdIndex] == PreviousInfo.strObjId)
            {
                Info.strName = PreviousInfo.strName + " -> ";
                break;
            }
        }

        Info.strName += Str1252ToStr(Record[NameIndex]);

        // Convert the RSRVD4_L column value to a size_t.
        // It describes an offset in the linkhrs.lnk file.
        auto Option = StrToSizeT(Record[Rsrvd4LIndex]);
        if (!Option.has_value())
        {
            return CS7PError(L"Invalid RSRVD4_L for " + StrToWstr(Info.strName) + L": " + StrToWstr(Record[Rsrvd4LIndex]));
        }

        size_t Offset = Option.value();

        // Read 512 bytes (as 128x uint32_t) at that offset from the linkhrs.lnk file.
        // See http://www.plctalk.net/qanda/showpost.php?p=355358&postcount=14
        Linkhrs.seekg(Offset);
        if (!Linkhrs)
        {
            return CS7PError(L"Could not seek to linkhrs.lnk offset " + std::to_wstring(Offset));
        }

        std::array<uint32_t, 128> Buffer;
        if (!Linkhrs.read(reinterpret_cast<char*>(Buffer.data()), sizeof(Buffer)))
        {
            return CS7PError(L"Could not read linkhrs.lnk offset " + std::to_wstring(Offset));
        }

        // Extract the Subblock List ID.
        auto it = std::find(Buffer.begin(), Buffer.end(), SubblockListIdMagic);
        if (it != Buffer.end() && ++it != Buffer.end())
        {
            Info.SubblockListId = *it;
        }

        // Extract the YDB ID.
        it = std::find(Buffer.begin(), Buffer.end(), SymbolListIdMagic);
        if (it != Buffer.end() && ++it != Buffer.end())
        {
            Info.SymbolListId = *it;
        }
    }

    return std::monostate();
}


std::variant<std::monostate, CS7PError>
ParseDeviceIdInfos(std::vector<S7DeviceIdInfo>& DeviceIdInfos, const std::wstring& wstrS7PFolderPath)
{
    static const std::string strStationRelID = "1315838";
    static const std::string strSecondLevelRelID = "16";

    const std::wstring wstrStationsDbfFilePath = wstrS7PFolderPath + L"\\hOmSave7\\s7hstatx\\HOBJECT1.DBF";
    const std::wstring wstrStationRelationsDbfFilePath = wstrS7PFolderPath + L"\\hOmSave7\\s7hstatx\\HRELATI1.DBF";

    const struct
    {
        std::wstring wstrDevicesDbfFilePath;
        std::wstring wstrDeviceRelationsDbfFilePath;
    }
    DeviceDbfInfos[] = {
        // S7-31x series of CPUs
        { wstrS7PFolderPath + L"\\hOmSave7\\S7HK31AX\\HOBJECT1.DBF", wstrS7PFolderPath + L"\\hOmSave7\\S7HK31AX\\HRELATI1.DBF" },

        // S7-41x series of CPUs
        { wstrS7PFolderPath + L"\\hOmSave7\\S7HK41AX\\HOBJECT1.DBF", wstrS7PFolderPath + L"\\hOmSave7\\S7HK41AX\\HRELATI1.DBF" },
    };

    std::vector<IntermediateInfo> PreviousInfosForResoff;
    if (_FileExists(wstrStationsDbfFilePath) && _FileExists(wstrStationRelationsDbfFilePath))
    {
        // Stations
        std::vector<IntermediateInfo> StationInfos;
        auto ParseResult = _ParseStations(StationInfos, wstrStationsDbfFilePath);
        if (const auto pError = std::get_if<CS7PError>(&ParseResult))
        {
            return CS7PError(L"Stations: " + pError->Message());
        }

        // Station -> Device
        std::vector<IntermediateInfo> StationRelationInfos;
        ParseResult = _ParseRelations(StationRelationInfos, wstrStationRelationsDbfFilePath, StationInfos, strStationRelID);
        if (const auto pError = std::get_if<CS7PError>(&ParseResult))
        {
            return CS7PError(L"Station relations: " + pError->Message());
        }

        // Devices
        for (const auto& DeviceDbfInfo : DeviceDbfInfos)
        {
            if (_FileExists(DeviceDbfInfo.wstrDevicesDbfFilePath) && _FileExists(DeviceDbfInfo.wstrDeviceRelationsDbfFilePath))
            {
                // Devices (CPUs only here)
                std::vector<IntermediateInfo> DeviceInfos;
                ParseResult = _ParseDevices(DeviceInfos, DeviceDbfInfo.wstrDevicesDbfFilePath, StationRelationInfos);
                if (const auto pError = std::get_if<CS7PError>(&ParseResult))
                {
                    return CS7PError(DeviceDbfInfo.wstrDevicesDbfFilePath + L": " + pError->Message());
                }

                // Device -> Device Content
                std::vector<IntermediateInfo> DeviceRelationInfos;
                ParseResult = _ParseRelations(DeviceRelationInfos, DeviceDbfInfo.wstrDeviceRelationsDbfFilePath, DeviceInfos, strSecondLevelRelID);
                if (const auto pError = std::get_if<CS7PError>(&ParseResult))
                {
                    return CS7PError(DeviceDbfInfo.wstrDeviceRelationsDbfFilePath + L": " + pError->Message());
                }

                PreviousInfosForResoff.insert(PreviousInfosForResoff.end(), DeviceRelationInfos.begin(), DeviceRelationInfos.end());
            }
        }
    }

    // Device Contents (S7 Programs only here)
    auto ParseResoffResult = _ParseResoffAndLinkhrs(DeviceIdInfos, wstrS7PFolderPath, PreviousInfosForResoff);
    if (const auto pError = std::get_if<CS7PError>(&ParseResoffResult))
    {
        return CS7PError(L"Resoff/Linkhrs: " + pError->Message());
    }

    return std::monostate();
}
