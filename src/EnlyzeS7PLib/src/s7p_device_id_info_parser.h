//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "CS7PError.h"

struct S7DeviceIdInfo
{
    std::string strName;
    std::optional<uint32_t> SubblockListId;
    std::optional<uint32_t> SymbolListId;
};

std::variant<std::monostate, CS7PError> ParseDeviceIdInfos(
    std::vector<S7DeviceIdInfo>& DeviceIdInfos,
    const std::wstring& wstrS7PFolderPath
    );
