//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#include <string>
#include <variant>
#include <vector>

#include "CS7PError.h"
#include "s7p_device_id_info_parser.h"
#include "s7p_parser.h"

std::variant<std::monostate, CS7PError> ParseOmbstx(
    std::vector<S7DeviceSymbolInfo>& DeviceSymbolInfos,
    const std::vector<S7DeviceIdInfo>& DeviceIdInfos,
    const std::wstring& wstrS7PFolderPath
    );
